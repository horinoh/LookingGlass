# LookingGlassCoreSDK with Vulkan, DirectX12 test

## ハードウエア (Hardware)
### Looking Glass Portrait
- USB-C, HDMI を PC に接続 (Connect USB-C and HDMI to PC)
- Portrait の電源を入れる (Power on portrait)
- スタート - 設定 - システム - ディスプレイ - マルチディスプレイ - 画面表示を拡張する - Looking Glass の画面を選択 - 拡大縮小とレイアウトを 100% にする (Start - Settings - System - Dsiplay - MultiDisplay - Extend these displays - Select Looking Glass Display - Scale and Layout - 100%) 

## 外部ライブラリ (Using libraries)

### [LookingGlassCoreSDK](https://github.com/Looking-Glass/LookingGlassCoreSDK)
- LookingGlassCoreSDK\HoloPlayCore\dylib\Win64 を環境変数 Path に通しておく (Add LookingGlassCoreSDK\HoloPlayCore\dylib\Win64 to environment variable Path)

### テクスチャ (Texture)
#### DX
- [DirectXTK12](https://github.com/Microsoft/DirectXTK12)
    - DirectXTK_Desktop_2022_Win10.sln を開いてビルド (Open and build DirectXTK_Desktop_2022_Win10.sln)
<!--
- [DirectXTex](https://github.com/microsoft/DirectXTex.git) ツール
    - DirectXTex_Desktop_2022.sln を開いてビルド
        - DDSView
        - Texassemble
        - Texconv
        - Texdiag
-->

#### VK
- [gli](https://github.com/g-truc/gli)

### メッシュ (Mesh)
#### [FBX](https://aps.autodesk.com/developer/overview/fbx-sdk)
- SDK をインストールして、環境変数 FBX_SDK_PATH をインストール先に設定しておく (Install SDK, and create environment varibale FBX_SDK_PATH)

#### [GLTF](https://www.nuget.org/packages/Microsoft.glTF.CPP)
- NuGetPackage で Microsoft.glTF.CPP をインストール (Install Microsoft.glTF.CPP from NuGetPackage)
- min, max 関連でコンパイルエラーになる場合、windows.h より前に NOMINMAX を定義する
    ~~~
    #define NOMINMAX 
    #include <windows.h>
    ~~~
- リンカエラー 4099 が出る(#pragma では回避できない)ので以下のようにしている
    - Configuration Properties - Linker - CommandLine - AdditionalOptions - /ignore:4099

### [OpenCV](https://github.com/opencv/opencv)
- インストール先を環境変数 OPENCV_SDK_PATH へセット (Create install folder as environment variable OPENCV_SDK_PATH)
- 環境変数 Path へ \$(OPENCV_SDK_PATH)\build\x64\vc16\bin を追加 (Add $(OPENCV_SDK_PATH)\build\x64\vc16\bin to environment variable Path)

## アセット (Assets)
- キルト画像(DDS)やメッシュ(FBX)は LookingGlass/Asset/ 以下へ配置しておく (Put assets(dds, fbx) in LookingGlass/Asset/)
- DDS のミップマップは不要 (No need mipmaps)
- RGBD 画像は RGBD/ 以下へ (Put RGBD images in folder RBGD/)
    
### キルト画像 (Quilt image)
- [Jane_Guan_Space_Nap_qs8x6.webp](https://docs.lookingglassfactory.com/keyconcepts/quilts) を ペイントツール等で png 等の形式で保存してから dds へ変換する (Convert Jane_Guan_Space_Nap_qs8x6.webp to dds format)

### メッシュ (Mesh)
- Thanks to [The Stanford 3D Scanning Repository](https://graphics.stanford.edu/data/3Dscanrep/)


