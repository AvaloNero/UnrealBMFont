// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "BMFontAsset.h"
#include "BMFontRendering.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontPackedChannelMappingTest,
	"UnrealBMFont.Rendering.PackedChannelMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontPackedChannelMappingTest::RunTest(const FString& Parameters)
{
	{
		FBMFontCommon Common;
		Common.bPacked = false;
		const FBMFontPackedChannelMapping Mapping = BMFontRendering::ComputePackedChannelMapping(Common, 2);
		TestTrue(
			TEXT("Non-packed descriptors sample alpha as coverage"),
			Mapping.ChannelWeights.Equals(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
		);
		TestEqual(TEXT("Non-packed descriptors have no constant bias"), Mapping.ConstantBias, 0.0f);
	}

	{
		FBMFontCommon Common;
		Common.bPacked = true;
		Common.AlphaChannel = EBMFontChannelContent::Outline;
		Common.RedChannel = EBMFontChannelContent::Zero;
		Common.GreenChannel = EBMFontChannelContent::Glyph;
		Common.BlueChannel = EBMFontChannelContent::Zero;
		const FBMFontPackedChannelMapping Mapping = BMFontRendering::ComputePackedChannelMapping(Common, 2);
		TestTrue(
			TEXT("Glyph channel becomes the coverage weight"),
			Mapping.ChannelWeights.Equals(FLinearColor(0.0f, 1.0f, 0.0f, 0.0f))
		);
		TestEqual(TEXT("Sampled coverage needs no bias"), Mapping.ConstantBias, 0.0f);
	}

	{
		FBMFontCommon Common;
		Common.bPacked = true;
		Common.AlphaChannel = EBMFontChannelContent::GlyphAndOutline;
		Common.RedChannel = EBMFontChannelContent::Zero;
		Common.GreenChannel = EBMFontChannelContent::Zero;
		Common.BlueChannel = EBMFontChannelContent::Zero;
		const FBMFontPackedChannelMapping Mapping = BMFontRendering::ComputePackedChannelMapping(Common, 8);
		TestTrue(
			TEXT("GlyphAndOutline channels count as coverage"),
			Mapping.ChannelWeights.Equals(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
		);
	}

	{
		FBMFontCommon Common;
		Common.bPacked = true;
		Common.AlphaChannel = EBMFontChannelContent::One;
		Common.RedChannel = EBMFontChannelContent::Zero;
		Common.GreenChannel = EBMFontChannelContent::Zero;
		Common.BlueChannel = EBMFontChannelContent::Zero;
		const FBMFontPackedChannelMapping Mapping = BMFontRendering::ComputePackedChannelMapping(Common, 8);
		TestTrue(
			TEXT("Always-one coverage drops sampling weights"),
			Mapping.ChannelWeights.Equals(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
		);
		TestEqual(TEXT("Always-one coverage becomes a constant bias"), Mapping.ConstantBias, 1.0f);
	}

	{
		FBMFontCommon Common;
		Common.bPacked = true;
		const FBMFontPackedChannelMapping Mapping = BMFontRendering::ComputePackedChannelMapping(Common, 2);
		TestTrue(
			TEXT("Unknown packed metadata falls back to the glyph-selected channel"),
			Mapping.ChannelWeights.Equals(FLinearColor(0.0f, 1.0f, 0.0f, 0.0f))
		);
		TestEqual(TEXT("Unknown packed metadata keeps zero bias"), Mapping.ConstantBias, 0.0f);
	}

	{
		FBMFontCommon Common;
		Common.bPacked = true;
		Common.AlphaChannel = EBMFontChannelContent::Glyph;
		Common.RedChannel = EBMFontChannelContent::Glyph;
		Common.GreenChannel = EBMFontChannelContent::Glyph;
		Common.BlueChannel = EBMFontChannelContent::Glyph;
		const FBMFontPackedChannelMapping Mapping = BMFontRendering::ComputePackedChannelMapping(Common, 1);
		TestTrue(
			TEXT("A glyph channel mask selects blue even when every channel carries glyphs"),
			Mapping.ChannelWeights.Equals(FLinearColor(0.0f, 0.0f, 1.0f, 0.0f))
		);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontPageRenderResourceTest,
	"UnrealBMFont.Rendering.PageRenderResource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontPageRenderResourceTest::RunTest(const FString& Parameters)
{
	const auto MakeAsset = [](const bool bPacked)
	{
		UBMFontAsset* Asset = NewObject<UBMFontAsset>();
		FBMFontData Data;
		Data.DescriptorFormat = EBMFontDescriptorFormat::Text;
		Data.Common.LineHeight = 8;
		Data.Common.Base = 8;
		Data.Common.ScaleWidth = 8;
		Data.Common.ScaleHeight = 8;
		Data.Common.PageCount = 1;
		Data.Common.bPacked = bPacked;
		if (bPacked)
		{
			Data.Common.AlphaChannel = EBMFontChannelContent::Zero;
			Data.Common.RedChannel = EBMFontChannelContent::Glyph;
			Data.Common.GreenChannel = EBMFontChannelContent::Glyph;
			Data.Common.BlueChannel = EBMFontChannelContent::Zero;
		}
		FBMFontPage& Page = Data.Pages.AddDefaulted_GetRef();
		Page.Id = 0;
		Page.File = TEXT("atlas.png");
		Page.Texture = NewObject<UTexture2D>(Asset);
		FBMFontGlyph& Glyph = Data.Glyphs.Add(65);
		Glyph.Codepoint = 65;
		Glyph.Width = 4;
		Glyph.Height = 4;
		Glyph.XAdvance = 4;
		Glyph.Page = 0;
		Glyph.Channel = 4;
		Asset->SetFontData(MoveTemp(Data));
		return Asset;
	};

	UBMFontAsset* PlainAsset = MakeAsset(false);
	TestEqual(
		TEXT("Non-packed pages render through their texture"),
		PlainAsset->GetPageRenderResource(0),
		static_cast<UObject*>(PlainAsset->GetPageTexture(0))
	);

	UBMFontAsset* PackedAsset = MakeAsset(true);
	UObject* PackedResource = PackedAsset->GetPageRenderResource(0, 4);
	UMaterialInstanceDynamic* MaterialInstance = Cast<UMaterialInstanceDynamic>(PackedResource);
	if (!TestNotNull(TEXT("Packed pages render through a dynamic material instance"), MaterialInstance))
	{
		return false;
	}

	UMaterialInterface* DefaultMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/UnrealBMFont/M_BMFontPacked.M_BMFontPacked")
	);
	if (!TestNotNull(TEXT("The plugin default packed material loads"), DefaultMaterial))
	{
		return false;
	}
	TestEqual(
		TEXT("Packed material instance parents the plugin default material"),
		MaterialInstance->Parent.Get(),
		DefaultMaterial
	);

	UTexture* AtlasTexture = nullptr;
	TestTrue(
		TEXT("Packed material instance binds the page texture"),
		MaterialInstance->GetTextureParameterValue(TEXT("FontAtlas"), AtlasTexture)
		&& AtlasTexture == PackedAsset->GetPageTexture(0)
	);

	FLinearColor Weights;
	TestTrue(
		TEXT("Packed material instance receives channel weights"),
		MaterialInstance->GetVectorParameterValue(TEXT("ChannelWeights"), Weights)
	);
	TestTrue(
		TEXT("Channel weights select the glyph's red channel"),
		Weights.Equals(FLinearColor(1.0f, 0.0f, 0.0f, 0.0f))
	);

	UMaterialInstanceDynamic* GreenMaterialInstance = Cast<UMaterialInstanceDynamic>(
		PackedAsset->GetPageRenderResource(0, 2)
	);
	if (!TestNotNull(TEXT("A second glyph channel resolves a material instance"), GreenMaterialInstance))
	{
		return false;
	}
	TestNotEqual(
		TEXT("Different glyph channels do not share a page-wide material instance"),
		GreenMaterialInstance,
		MaterialInstance
	);
	TestTrue(
		TEXT("Green glyphs receive green channel weights"),
		GreenMaterialInstance->GetVectorParameterValue(TEXT("ChannelWeights"), Weights)
		&& Weights.Equals(FLinearColor(0.0f, 1.0f, 0.0f, 0.0f))
	);

	TestEqual(
		TEXT("Repeated resolution reuses the cached instance"),
		PackedAsset->GetPageRenderResource(0, 4),
		static_cast<UObject*>(MaterialInstance)
	);

	return true;
}

#endif
