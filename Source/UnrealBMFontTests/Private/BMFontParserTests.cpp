// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "BMFontParser.h"
#include "Misc/AutomationTest.h"

namespace
{
	void AddUInt8(TArray<uint8>& Bytes, const uint8 Value)
	{
		Bytes.Add(Value);
	}

	void AddUInt16(TArray<uint8>& Bytes, const uint16 Value)
	{
		Bytes.Add(static_cast<uint8>(Value & 0xFF));
		Bytes.Add(static_cast<uint8>((Value >> 8) & 0xFF));
	}

	void AddInt16(TArray<uint8>& Bytes, const int16 Value)
	{
		AddUInt16(Bytes, static_cast<uint16>(Value));
	}

	void AddUInt32(TArray<uint8>& Bytes, const uint32 Value)
	{
		Bytes.Add(static_cast<uint8>(Value & 0xFF));
		Bytes.Add(static_cast<uint8>((Value >> 8) & 0xFF));
		Bytes.Add(static_cast<uint8>((Value >> 16) & 0xFF));
		Bytes.Add(static_cast<uint8>((Value >> 24) & 0xFF));
	}

	void AddUtf8String(TArray<uint8>& Bytes, const ANSICHAR* Value)
	{
		while (*Value != '\0')
		{
			Bytes.Add(static_cast<uint8>(*Value++));
		}
		Bytes.Add(0);
	}

	void AddBlock(TArray<uint8>& File, const uint8 Type, const TArray<uint8>& Block)
	{
		AddUInt8(File, Type);
		AddUInt32(File, Block.Num());
		File.Append(Block);
	}

	TArray<uint8> MakeBinaryDescriptor(const bool bAddDuplicateKerning = false)
	{
		TArray<uint8> File = { 'B', 'M', 'F', 3 };

		TArray<uint8> Info;
		AddInt16(Info, 24);
		AddUInt8(Info, 0x03);
		AddUInt8(Info, 0);
		AddUInt16(Info, 100);
		AddUInt8(Info, 1);
		AddUInt8(Info, 1);
		AddUInt8(Info, 2);
		AddUInt8(Info, 3);
		AddUInt8(Info, 4);
		AddUInt8(Info, 1);
		AddUInt8(Info, 1);
		AddUInt8(Info, 0);
		AddUtf8String(Info, "Binary Test");
		AddBlock(File, 1, Info);

		TArray<uint8> Common;
		AddUInt16(Common, 32);
		AddUInt16(Common, 24);
		AddUInt16(Common, 64);
		AddUInt16(Common, 64);
		AddUInt16(Common, 1);
		AddUInt8(Common, 0);
		AddUInt8(Common, 0);
		AddUInt8(Common, 4);
		AddUInt8(Common, 4);
		AddUInt8(Common, 4);
		AddBlock(File, 2, Common);

		TArray<uint8> Pages;
		AddUtf8String(Pages, "atlas.png");
		AddBlock(File, 3, Pages);

		TArray<uint8> Chars;
		AddUInt32(Chars, 65);
		AddUInt16(Chars, 2);
		AddUInt16(Chars, 3);
		AddUInt16(Chars, 8);
		AddUInt16(Chars, 10);
		AddInt16(Chars, -1);
		AddInt16(Chars, 2);
		AddInt16(Chars, 9);
		AddUInt8(Chars, 0);
		AddUInt8(Chars, 15);
		AddBlock(File, 4, Chars);

		TArray<uint8> Kernings;
		AddUInt32(Kernings, 65);
		AddUInt32(Kernings, 65);
		AddInt16(Kernings, -2);
		if (bAddDuplicateKerning)
		{
			AddUInt32(Kernings, 65);
			AddUInt32(Kernings, 65);
			AddInt16(Kernings, -3);
		}
		AddBlock(File, 5, Kernings);
		return File;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontTextDescriptorTest,
	"UnrealBMFont.Parser.TextDescriptor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontTextDescriptorTest::RunTest(const FString& Parameters)
{
	const FString Descriptor =
		TEXT("\xFEFFinfo face=\"Open Font\" size=32 bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=1 padding=1,2,3,4 spacing=5,6 outline=1\n")
		TEXT("common pages=2 scaleH=64 lineHeight=32 scaleW=128 base=24 packed=0 alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4\n")
		TEXT("page file=\"atlas zero.png\" id=0\n")
		TEXT("page id=1 file=\"atlas-one.png\"\n")
		TEXT("chars count=3\n")
		TEXT("char xadvance=8 id=32 x=0 y=0 width=0 height=0 xoffset=0 yoffset=0 page=0 chnl=15\n")
		TEXT("char id=65 x=1 y=2 width=8 height=10 xoffset=-1 yoffset=3 xadvance=11 page=0 chnl=15\n")
		TEXT("char id=128512 x=16 y=2 width=12 height=12 xoffset=0 yoffset=1 xadvance=13 page=1 chnl=15\n")
		TEXT("kernings count=1\n")
		TEXT("kerning second=65 amount=-2 first=65\n");

	const FBMFontParseResult Result = FBMFontParser::ParseText(Descriptor);
	TestTrue(TEXT("Text descriptor parses"), Result.IsSuccess());
	TestEqual(TEXT("Format"), Result.Data.DescriptorFormat, EBMFontDescriptorFormat::Text);
	TestEqual(TEXT("Face preserves spaces"), Result.Data.Info.Face, FString(TEXT("Open Font")));
	TestEqual(TEXT("Padding right"), Result.Data.Info.PaddingRight, 2);
	TestEqual(TEXT("Spacing"), Result.Data.Info.Spacing, FIntPoint(5, 6));
	TestEqual(TEXT("Page count"), Result.Data.Pages.Num(), 2);
	TestEqual(TEXT("Glyph count"), Result.Data.Glyphs.Num(), 3);
	TestTrue(TEXT("Supplementary Unicode glyph exists"), Result.Data.Glyphs.Contains(0x1F600));
	const FBMFontGlyph* Glyph = Result.Data.Glyphs.Find(65);
	TestNotNull(TEXT("ASCII glyph exists"), Glyph);
	if (Glyph != nullptr)
	{
		TestEqual(TEXT("Field order is independent"), Glyph->XAdvance, 11);
	}
	TestEqual(TEXT("Kerning count"), Result.Data.KerningPairs.Num(), 1);
	if (!Result.Data.KerningPairs.IsEmpty())
	{
		TestEqual(TEXT("Kerning amount"), Result.Data.KerningPairs[0].Amount, -2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontXmlDescriptorTest,
	"UnrealBMFont.Parser.XmlDescriptor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontXmlDescriptorTest::RunTest(const FString& Parameters)
{
	const FString Xml =
		TEXT("<?xml version=\"1.0\"?>")
		TEXT("<font><info face=\"XML Font\" size=\"16\" unicode=\"1\"/>")
		TEXT("<common lineHeight=\"20\" base=\"15\" scaleW=\"64\" scaleH=\"64\" pages=\"1\" packed=\"0\"/>")
		TEXT("<pages><page id=\"0\" file=\"atlas.png\"/></pages>")
		TEXT("<chars count=\"1\"><char id=\"65\" x=\"1\" y=\"2\" width=\"8\" height=\"10\" xoffset=\"0\" yoffset=\"1\" xadvance=\"9\" page=\"0\" chnl=\"15\"/></chars>")
		TEXT("<kernings count=\"0\"/></font>");

	const FBMFontParseResult Result = FBMFontParser::ParseXml(Xml);
	TestTrue(TEXT("XML descriptor parses"), Result.IsSuccess());
	TestEqual(TEXT("Format"), Result.Data.DescriptorFormat, EBMFontDescriptorFormat::Xml);
	TestEqual(TEXT("Face"), Result.Data.Info.Face, FString(TEXT("XML Font")));
	const FBMFontGlyph* Glyph = Result.Data.Glyphs.Find(65);
	TestNotNull(TEXT("Glyph exists"), Glyph);
	if (Glyph != nullptr)
	{
		TestEqual(TEXT("Glyph advance"), Glyph->XAdvance, 9);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontBinaryDescriptorTest,
	"UnrealBMFont.Parser.BinaryDescriptor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontBinaryDescriptorTest::RunTest(const FString& Parameters)
{
	const TArray<uint8> Descriptor = MakeBinaryDescriptor();
	const FBMFontParseResult Result = FBMFontParser::Parse(Descriptor);
	TestTrue(TEXT("Binary descriptor parses"), Result.IsSuccess());
	TestEqual(TEXT("Format"), Result.Data.DescriptorFormat, EBMFontDescriptorFormat::Binary);
	TestEqual(TEXT("Face"), Result.Data.Info.Face, FString(TEXT("Binary Test")));
	const FBMFontGlyph* Glyph = Result.Data.Glyphs.Find(65);
	TestNotNull(TEXT("Binary glyph exists"), Glyph);
	if (Glyph != nullptr)
	{
		TestEqual(TEXT("Glyph offset"), Glyph->XOffset, -1);
	}
	TestEqual(TEXT("Binary kerning count"), Result.Data.KerningPairs.Num(), 1);
	if (!Result.Data.KerningPairs.IsEmpty())
	{
		TestEqual(TEXT("Kerning"), Result.Data.KerningPairs[0].Amount, -2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontBinaryDuplicateKerningTest,
	"UnrealBMFont.Parser.BinaryDuplicateKerning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontBinaryDuplicateKerningTest::RunTest(const FString& Parameters)
{
	const FBMFontParseResult Result = FBMFontParser::Parse(MakeBinaryDescriptor(true));
	TestTrue(TEXT("Descriptor with duplicate binary kerning parses"), Result.IsSuccess());
	TestEqual(TEXT("Duplicate pair is deduplicated"), Result.Data.KerningPairs.Num(), 1);
	if (!Result.Data.KerningPairs.IsEmpty())
	{
		TestEqual(TEXT("The last binary kerning record wins"), Result.Data.KerningPairs[0].Amount, -3);
	}
	TestTrue(
		TEXT("Duplicate binary kerning emits a warning"),
		Result.Messages.ContainsByPredicate(
			[](const FBMFontParseMessage& Message)
			{
				return Message.Severity == EBMFontParseMessageSeverity::Warning
					&& Message.Message.Contains(TEXT("Duplicate kerning pair"));
			}
		)
	);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontInvalidDescriptorTest,
	"UnrealBMFont.Parser.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontInvalidDescriptorTest::RunTest(const FString& Parameters)
{
	const FString Descriptor =
		TEXT("common lineHeight=0 base=0 scaleW=0 scaleH=0 pages=1 packed=0\n")
		TEXT("page id=0 file=\"atlas.png\"\n")
		TEXT("chars count=1\n")
		TEXT("char id=65 x=0 y=0 width=8 height=8 xoffset=0 yoffset=0 xadvance=8 page=0 chnl=15\n");
	const FBMFontParseResult Result = FBMFontParser::ParseText(Descriptor);
	TestFalse(TEXT("Invalid atlas is rejected"), Result.IsSuccess());
	TestTrue(TEXT("Validation emits errors"), Result.HasErrors());
	return true;
}

#endif
