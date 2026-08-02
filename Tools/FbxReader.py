#!/usr/bin/env python3
"""Minimal reader for binary FBX 7.x files, scoped to static low-poly meshes.

The Cars-Park pack is Blender-exported FBX: flat-shaded meshes, per-polygon
materials, no textures, no skinning, no animation. This reads exactly that and
raises on anything it was not built for, so a surprising input fails the cook
instead of producing a silently wrong model.

Everything it returns is already in the game's coordinate system: metres,
Y up, X right, Z toward the viewer (so a car facing +Z drives out of the
screen). The axis mapping is not hardcoded - it comes from the file's own
GlobalSettings, which is what makes the result verifiable against the OBJ.
"""

import struct
import zlib

FBX_MAGIC = b"Kaydara FBX Binary  \x00"

_SCALAR_PROPS = {b"Y": ("h", 2), b"C": ("?", 1), b"I": ("i", 4),
                 b"F": ("f", 4), b"D": ("d", 8), b"L": ("q", 8)}
_ARRAY_PROPS = {b"f": "f", b"d": "d", b"l": "q", b"i": "i", b"b": "?"}


class FbxError(Exception):
    pass


class Node:
    def __init__(self, name, props):
        self.name = name
        self.props = props
        self.children = []

    def Find(self, name):
        for child in self.children:
            if child.name == name:
                return child
        return None

    def FindAll(self, name):
        return [child for child in self.children if child.name == name]


def _ReadProperty(data, offset):
    kind = data[offset:offset + 1]
    offset += 1

    if kind in _SCALAR_PROPS:
        fmt, size = _SCALAR_PROPS[kind]
        return struct.unpack_from("<" + fmt, data, offset)[0], offset + size

    if kind in _ARRAY_PROPS:
        count, encoding, compressedLength = struct.unpack_from("<III", data, offset)
        offset += 12
        payload = data[offset:offset + compressedLength]
        offset += compressedLength
        if encoding == 1:
            payload = zlib.decompress(payload)
        elif encoding != 0:
            raise FbxError("unsupported array encoding %d" % encoding)
        return list(struct.unpack_from("<%d%s" % (count, _ARRAY_PROPS[kind]), payload, 0)), offset

    if kind in (b"S", b"R"):
        length = struct.unpack_from("<I", data, offset)[0]
        offset += 4
        value = data[offset:offset + length]
        offset += length
        return (value.decode("utf-8", "replace") if kind == b"S" else value), offset

    raise FbxError("unknown property type %r at byte %d" % (kind, offset))


def _ReadNode(data, offset, wide):
    # Records are uint32 before FBX 7500 and uint64 from 7500 on.
    fmt, size = ("<QQQB", 25) if wide else ("<IIIB", 13)
    endOffset, propertyCount, _propertyBytes, nameLength = struct.unpack_from(fmt, data, offset)
    offset += size
    if endOffset == 0:
        return None, offset  # the null record that terminates a child list

    name = data[offset:offset + nameLength].decode("utf-8", "replace")
    offset += nameLength

    props = []
    for _ in range(propertyCount):
        value, offset = _ReadProperty(data, offset)
        props.append(value)

    node = Node(name, props)
    while offset < endOffset - size:
        child, offset = _ReadNode(data, offset, wide)
        if child is None:
            break
        node.children.append(child)
    return node, endOffset


def ReadTree(path):
    """Parses an FBX file into a list of top level nodes."""
    with open(path, "rb") as handle:
        data = handle.read()

    if not data.startswith(FBX_MAGIC):
        raise FbxError("not a binary FBX file (ASCII FBX is not supported)")

    version = struct.unpack_from("<I", data, 23)[0]
    if version < 7100:
        raise FbxError("FBX version %d is too old, 7100+ expected" % version)

    offset = 27
    wide = version >= 7500
    roots = []
    while offset < len(data) - (25 if wide else 13):
        node, offset = _ReadNode(data, offset, wide)
        if node is None:
            break
        roots.append(node)
    return roots


def _Properties(node):
    """Flattens a Properties70 block into {name: [values]}."""
    block = node.Find("Properties70")
    if block is None:
        return {}
    return {p.props[0]: p.props[4:] for p in block.children if p.name == "P"}


def _ObjectName(node):
    # Object names are "Name\x00\x01ClassName".
    return node.props[1].split("\x00")[0]


def _AxisConverter(roots):
    """Builds the FBX -> game axis remap from the file's own GlobalSettings.

    FBX states which of its axes is up, front and right (the "coord" axis).
    The game uses X right, Y up, Z toward the viewer, so each output axis is
    just a signed pick of an input axis. Positions are also converted from
    FBX's centimetre base unit to metres.
    """
    settings = None
    for root in roots:
        if root.name == "GlobalSettings":
            settings = _Properties(root)
    if settings is None:
        raise FbxError("file has no GlobalSettings")

    def Axis(name):
        return int(settings[name][0]), int(settings[name + "Sign"][0])

    right, rightSign = Axis("CoordAxis")
    up, upSign = Axis("UpAxis")
    front, frontSign = Axis("FrontAxis")
    scale = float(settings.get("UnitScaleFactor", [1.0])[0]) / 100.0

    picks = ((right, rightSign), (up, upSign), (front, frontSign))
    if sorted(axis for axis, _ in picks) != [0, 1, 2]:
        raise FbxError("GlobalSettings does not name three distinct axes")

    def Convert(v, distance=True):
        factor = scale if distance else 1.0
        return tuple(v[axis] * sign * factor for axis, sign in picks)

    return Convert


def _Triangulate(geometry, materialCount):
    """Expands one Geometry node into flat-shaded triangles.

    Returns a list of (materialIndex, [(position, normal) * 3]) triangles, all
    still in raw FBX space - the caller applies the axis conversion once.
    """
    rawVertices = geometry.Find("Vertices").props[0]
    polygonIndices = geometry.Find("PolygonVertexIndex").props[0]

    normalLayer = geometry.Find("LayerElementNormal")
    if normalLayer is None:
        raise FbxError("geometry has no normals")
    if normalLayer.Find("MappingInformationType").props[0] != "ByPolygonVertex" or \
       normalLayer.Find("ReferenceInformationType").props[0] != "Direct":
        raise FbxError("only ByPolygonVertex/Direct normals are supported")
    rawNormals = normalLayer.Find("Normals").props[0]

    materialLayer = geometry.Find("LayerElementMaterial")
    perPolygonMaterial = None
    if materialLayer is not None:
        mapping = materialLayer.Find("MappingInformationType").props[0]
        if mapping == "ByPolygon":
            perPolygonMaterial = materialLayer.Find("Materials").props[0]
        elif mapping == "AllSame":
            perPolygonMaterial = None
            singleMaterial = materialLayer.Find("Materials").props[0][0]
        else:
            raise FbxError("unsupported material mapping '%s'" % mapping)

    def Vertex(index):
        return (rawVertices[index * 3], rawVertices[index * 3 + 1], rawVertices[index * 3 + 2])

    def Normal(cornerIndex):
        return (rawNormals[cornerIndex * 3], rawNormals[cornerIndex * 3 + 1], rawNormals[cornerIndex * 3 + 2])

    triangles = []
    polygon = []       # (vertexIndex, polygonVertexIndex) for the current face
    polygonIndex = 0
    for corner, index in enumerate(polygonIndices):
        last = index < 0
        polygon.append((~index if last else index, corner))
        if not last:
            continue

        material = 0
        if perPolygonMaterial is not None:
            material = perPolygonMaterial[polygonIndex]
        elif materialLayer is not None:
            material = singleMaterial
        if material < 0 or material >= materialCount:
            raise FbxError("polygon %d references material %d of %d" %
                           (polygonIndex, material, materialCount))

        # Fan triangulation. These are convex low-poly faces (quads and cylinder
        # caps), so a fan from the first corner is exact.
        for k in range(1, len(polygon) - 1):
            corners = (polygon[0], polygon[k], polygon[k + 1])
            triangles.append((material, [(Vertex(v), Normal(c)) for v, c in corners]))

        polygon = []
        polygonIndex += 1

    if polygon:
        raise FbxError("polygon index list ends mid-face")
    return triangles


def ReadMeshes(path):
    """Reads every mesh in an FBX file, baked into the game's coordinate system.

    Returns (materials, triangles):
      materials  list of {"name", "diffuse" (linear r,g,b)}, deduplicated by name
      triangles  list of (materialIndex, [(position, normal) * 3])
    """
    roots = ReadTree(path)
    Convert = _AxisConverter(roots)

    objects = None
    connections = None
    for root in roots:
        if root.name == "Objects":
            objects = root
        elif root.name == "Connections":
            connections = root
    if objects is None or connections is None:
        raise FbxError("file has no Objects/Connections section")

    nodesById = {}
    for node in objects.children:
        if node.props and isinstance(node.props[0], int):
            nodesById[node.props[0]] = node

    # Object-to-object links, kept in file order: a mesh's per-polygon material
    # index refers to the order its materials are connected to its model.
    childrenOf = {}
    for link in connections.children:
        if link.name != "C" or link.props[0] != "OO":
            continue
        childrenOf.setdefault(link.props[2], []).append(link.props[1])

    materials = []
    materialIndexByName = {}
    triangles = []

    for model in objects.FindAll("Model"):
        modelId = model.props[0]
        properties = _Properties(model)
        for unsupported in ("Lcl Rotation", "Lcl Scaling", "PreRotation", "GeometricTranslation"):
            if unsupported in properties:
                raise FbxError("model '%s' uses %s, which this reader does not bake" %
                               (_ObjectName(model), unsupported))

        translation = (0.0, 0.0, 0.0)
        if "Lcl Translation" in properties:
            translation = Convert([float(v) for v in properties["Lcl Translation"]])

        geometry = None
        localMaterials = []
        for childId in childrenOf.get(modelId, []):
            child = nodesById.get(childId)
            if child is None:
                continue
            if child.name == "Geometry":
                geometry = child
            elif child.name == "Material":
                name = _ObjectName(child)
                if name not in materialIndexByName:
                    diffuse = _Properties(child).get("DiffuseColor", [0.8, 0.8, 0.8])
                    factor = float(_Properties(child).get("DiffuseFactor", [1.0])[0])
                    materialIndexByName[name] = len(materials)
                    materials.append({"name": name,
                                      "diffuse": tuple(float(c) * factor for c in diffuse[:3])})
                localMaterials.append(materialIndexByName[name])

        if geometry is None:
            continue

        for localMaterial, corners in _Triangulate(geometry, len(localMaterials)):
            baked = []
            for position, normal in corners:
                position = Convert(position)
                normal = Convert(normal, distance=False)
                baked.append((tuple(position[i] + translation[i] for i in range(3)), normal))
            triangles.append((localMaterials[localMaterial], baked))

    return materials, triangles
