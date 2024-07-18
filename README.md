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

<!--
#### [GLTF](https://www.nuget.org/packages/Microsoft.glTF.CPP)
- NuGetPackage で Microsoft.glTF.CPP をインストール (Install Microsoft.glTF.CPP from NuGetPackage)
- min, max 関連でコンパイルエラーになる場合、windows.h より前に NOMINMAX を定義する
    ~~~
    #define NOMINMAX 
    #include <windows.h>
    ~~~
- リンカエラー 4099 が出る(#pragma では回避できない)ので以下のようにしている
    - Configuration Properties - Linker - CommandLine - AdditionalOptions - /ignore:4099
-->

### 画像処理 (Image processing)
#### [OpenCV](https://github.com/horinoh/OpenCV/tree/master/)
- インストール先を環境変数 OPENCV_SDK_PATH へセット (Create environment variable OPENCV_SDK_PATH)
- 環境変数 Path へ `$(OPENCV_SDK_PATH)\build\x64\vc17\bin` を追加 (Add `$(OPENCV_SDK_PATH)\build\x64\vc17\bin` to environment variable Path)

### 深度センサ (Depth sensor)
#### [MaixSenseA010](https://wiki.sipeed.com/hardware/en/maixsense/maixsense-a010/maixsense-a010.html)
- デプスセンサーとして使用 (Used as depth sensor)
- [comtool](https://dl.sipeed.com/shareURL/MaixSense/MaixSense_A010/software_pack/comtool) をインストールしておく (Install comtool)

#### [MaixSenseA075v](https://wiki.sipeed.com/hardware/en/maixsense/maixsense-a075v/maixsense-a075v.html)
- ↑ページを参考にドライバをインストールしておく (Install driver)
- [192.168.233.1](http://192.168.233.1/) へアクセスして動作確認 (Access 192.168.233.1 to check)
 
#### [asio](https://github.com/chriskohlhoff/asio.git)
- シリアルポートや Http で使用 (Use to handle serial port, http)
- サブモジュールとして追加した (Add as submodule)

<!--
### [LeapMotion]()
- インストール先を環境変数 LEAP_SDK_PATH へセット (Create install folder as environment variable LEAP_SDK_PATH)
-->

## アセット (Assets)
- アセットは LookingGlass/Asset/ 以下へ配置しておく (Put assets(dds, fbx) in LookingGlass/Asset/)
    - Mesh/ メッシュ
    - Quilt/ キルト画像
    - RGBD/ RGBD 画像
    - RGB_D/ RGB, Depth 画像 (DDS)
        - DDS のミップマップは不要 (No need mipmaps)
### Thanks to
- [The Stanford 3D Scanning Repository](https://graphics.stanford.edu/data/3Dscanrep/)
- [cc0textures](https://cc0textures.com/)

## プロジェクト (Projects)
- QulitVK, QuiltDX
    - キルト画像を Looking Glass (LKG) へ表示
- MeshVK, MeshDX
    - メッシュを複数視点から描画しキルト画像を作成
    - LKG へ表示
- Mesh2VK, Mesh2DX
    - 上記に加え、メッシュを複数表示(インスタンシング)、回転させている
- DisplacementVK, DisplacementDX
    - デプスマップで板メッシュをディスプレースメントする
    - 複数視点描画しキルト画像を作成
    - LKG へ表示
- DepthSensorVK, DepthSensorDX
    - 基本的には Displacement と同じ
    - 要深度センサ (MaixSense A010) 、センサの出力をデプスマップとして使用している
    - ディスプレースメント値を極端にした方が効果が分かりやすい
