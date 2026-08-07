# Copyright (c) 2026 AvaloNero. Licensed under the MIT License.
#
# Generates the plugin's packed-atlas UI material. Run inside Unreal Editor with the
# Python Script Plugin enabled and the UnrealBMFont plugin mounted:
#
#   UnrealEditor-Cmd.exe <HostProject> -ExecutePythonScript=<repo>\Scripts\GeneratePackedMaterial.py `
#     -Unattended -NoSplash -NoSound -stdout -FullStdOutLogOutput
#
# The material is written to the plugin's mounted content root as
# /UnrealBMFont/M_BMFontPacked and must be committed to Content/M_BMFontPacked.uasset.
# Re-run and eyeball the diff whenever the parameter contract changes.

import unreal

MATERIAL_PATH = "/UnrealBMFont/M_BMFontPacked"

PARAM_ATLAS = "FontAtlas"
PARAM_WEIGHTS = "ChannelWeights"
PARAM_BIAS = "ChannelBias"


def log(message):
    unreal.log("GeneratePackedMaterial: {}".format(message))


def set_parameter_name(expression, name):
    expression.set_editor_property("parameter_name", name)


def main():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material_library = unreal.MaterialEditingLibrary
    asset_library = unreal.EditorAssetLibrary

    package_path, _, asset_name = MATERIAL_PATH.rpartition("/")

    # The commandlet asset registry may not know an on-disk package, so check by
    # direct load rather than does_asset_exist.
    existing = unreal.load_object(None, "{}.{}".format(MATERIAL_PATH, asset_name))
    if existing is not None:
        log("deleting existing material for regeneration: {}".format(MATERIAL_PATH))
        unreal.EditorAssetLibrary.delete_loaded_asset(existing)

    material = asset_tools.create_asset(
        asset_name,
        package_path,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None:
        raise RuntimeError("failed to create material asset at {}".format(MATERIAL_PATH))

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    atlas = material_library.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0
    )
    set_parameter_name(atlas, PARAM_ATLAS)
    atlas.set_editor_property("desc", "Packed atlas page texture")

    weights = material_library.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -600, 220
    )
    set_parameter_name(weights, PARAM_WEIGHTS)
    weights.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    weights.set_editor_property("desc", "Per-channel coverage weights")

    dot = material_library.create_material_expression(
        material, unreal.MaterialExpressionDotProduct, -320, 40
    )
    material_library.connect_material_expressions(atlas, "RGBA", dot, "A")
    material_library.connect_material_expressions(weights, "", dot, "B")

    bias = material_library.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -600, 400
    )
    set_parameter_name(bias, PARAM_BIAS)
    bias.set_editor_property("default_value", 0.0)
    bias.set_editor_property("desc", "Constant coverage for always-one channels")

    coverage = material_library.create_material_expression(
        material, unreal.MaterialExpressionAdd, -120, 120
    )
    material_library.connect_material_expressions(dot, "", coverage, "A")
    material_library.connect_material_expressions(bias, "", coverage, "B")

    # Glyph coverage is grayscale; the widget's vertex color applies the tint.
    white = material_library.create_material_expression(
        material, unreal.MaterialExpressionConstant, -320, 320
    )
    white.set_editor_property("r", 1.0)

    material_library.connect_material_property(white, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    material_library.connect_material_property(coverage, "", unreal.MaterialProperty.MP_OPACITY)

    material_library.recompile_material(material)
    if not asset_library.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError("failed to save material asset: {}".format(MATERIAL_PATH))

    log("saved {} with parameters {}, {}, {}".format(
        MATERIAL_PATH, PARAM_ATLAS, PARAM_WEIGHTS, PARAM_BIAS))


if __name__ == "__main__":
    main()
else:
    main()
