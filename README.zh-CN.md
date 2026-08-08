<p align="center">
  <img src="Resources/Icon128.png" width="96" height="96" alt="Unreal BMFont 标志">
</p>

# Unreal BMFont

Unreal BMFont 用于导入 AngelCode BMFont 描述文件，并在 Slate / UMG 中渲染位图字形。核心控件是为 BMFont 专门设计的普通文本控件，可以把它理解为 `UTextBlock` 的位图字体版本，并使用精简的专用布局与渲染管线。

插件当前版本为 Beta `0.2.0`。Runtime API 已保持精简可用，但不会宣称支持尚未验证的引擎版本和平台。

[English](README.md) · [格式支持](Docs/FormatSupport.md) · [架构](Docs/Architecture.md) · [测试](Docs/Testing.md)

![Unreal BMFont 多语言运行时展示](Docs/Images/showcase-runtime.png)

## 主要能力

- 支持文本、XML 和二进制 v3 三种 `.fnt` 描述格式。
- 按描述文件中的真实文件名导入纹理，支持多页图集。
- 保留 Unicode 码点、字形偏移、步进、行高以及 kerning 数据。
- 提供 `UBMFontAsset`、`UBMFontText`、底层 `SBMFontText`，以及 Rich Text 适配控件 `UBMFontRichTextBlock`。
- Packed-channel 图集通过通道提取 UI 材质渲染，并遵循每个字形的 `char.chnl` 掩码。
- 支持 Rich Text 中的 BMFont run，包括空标签和省略号溢出策略。
- 支持换行、对齐、边距、行高、字间距、染色、阴影、缺字回退、文本绑定和像素对齐。
- 支持资产重导入，并保留用户自行修改的纹理过滤方式。
- 提供编辑器缩略图和带字形框的只读字体/图集检查器。
- Runtime、Editor/Importer 与自动化测试模块彼此隔离。
- 对错误描述文件、越界图集、危险页面路径和纹理尺寸不匹配给出明确日志。

## 环境要求

- Unreal Engine 5.8
- C++ 项目，或已预编译的插件包

目前完整验证的平台是 Win64。其它版本和平台请先查看[兼容性说明](Docs/Compatibility.md)。

## 安装

1. 将仓库复制到 `<项目目录>/Plugins/UnrealBMFont`。
2. 源码工程需要时重新生成项目文件。
3. 编译项目，在插件窗口启用 **Unreal BMFont**。
4. 按编辑器提示重启。

生成可分发插件包：

```powershell
pwsh ./Scripts/BuildPlugin.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8"
```

该脚本会建立一份排除已知本地构建产物的干净暂存副本，默认把结果写入 `%TEMP%` 下带时间戳的目录。也可以用 `-OutputDirectory` 指定插件源码树之外的其它空目录。

## 使用

1. 导出 AngelCode BMFont，并把 `.fnt` 引用的所有图集图片放在描述文件所在目录或其子目录。
2. 在 Content Browser 中导入 `.fnt`；插件会创建 `BMFont` 资产并导入页面纹理。
3. 在 UMG 面板中加入 **BMFont Text**。
4. 指定导入的字体资产并设置 `Text`。
5. 如果图集不含替换字符 `U+FFFD`，把 `Fallback Codepoint` 改成图集中已有的字符，常用值是问号 `63`。

仓库在 [`Samples/Minimal`](Samples/Minimal) 中提供了一套完全原创的最小导入样例，并在 [`Samples/Showcase`](Samples/Showcase) 中提供多语言人工验收样例。Showcase 覆盖数字、大写拉丁字母、平假名、中文数字和缺字回退。

## 明确的边界

BMFont 保存的是已经栅格化的字形矩形，不是字体塑形系统。当前版本适用于拉丁文、CJK、数字、图标和其它从左到右的预制字形集；不提供双向排版、OpenType shaping 或连字。Rich Text 适配器按 run 独立测量，kerning 不跨标签边界。Packed-channel 图集通过内置通道提取材质渲染，outline 通道不做单独合成。

完整边界见[格式支持](Docs/FormatSupport.md)，后续方向见[路线图](ROADMAP.md)。

## 开发与验证

先运行不依赖 Unreal Engine 的仓库卫生检查：

```powershell
python ./Scripts/ValidateRepository.py
```

自动化用例覆盖描述解析、Unicode 码点、kerning、换行、行高、真实 PNG 导入、纹理默认值和重导入。可以对装有插件的任意宿主项目运行：

```powershell
pwsh ./Scripts/TestPlugin.ps1 `
  -EngineRoot "C:\Program Files\Epic Games\UE_5.8" `
  -Project "D:\Projects\BMFontHost\BMFontHost.uproject"
```

提交改动前请阅读[贡献指南](CONTRIBUTING.md)。

## 许可证

Unreal BMFont 使用 [MIT License](LICENSE)。仓库不包含第三方源码或字体文件；生成后的 Showcase 图集来源见[第三方声明](THIRD_PARTY_NOTICES.md)。
