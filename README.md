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
#### [OpenCV](https://github.com/opencv/opencv)
- インストール先を環境変数 OPENCV_SDK_PATH へセット (Create install folder as environment variable OPENCV_SDK_PATH)
- 環境変数 Path へ `$(OPENCV_SDK_PATH)\build\x64\vc16\bin` を追加 (Add `$(OPENCV_SDK_PATH)\build\x64\vc16\bin` to environment variable Path)

#### [OpenCV](https://github.com/opencv/opencv) (ビルドする場合)
- [CMake](https://cmake.org/)
    - "Where is the source code" に [opencv](https://github.com/opencv/opencv) をクローンしたレポジトリを指定
    - "Where to build the binaries" に適当なフォルダ (**CVPATH** とする) を指定する 
    - オプション
        - 静的ライブラリにする場合 
            - BUILD_SHARED_LIBS のチェックを外す
        - ライブラリが大量にできて使用時に大変なのでまとめる場合
            - BUILD_opencv_world にチェックを入れる
        - CUDA を使用する場合
            - CUDA は予めインストールしておく
            - OPENCV_EXTRA_MODULES_PATH に クローンした [contrib](https://github.com/opencv/opencv_contrib) 以下の mudules フォルダを指定する
            - WITH_CUDA にチェックを入れる
- ビルド
    - ALL_BUILD (Debug, Release) を行う
    - INSTALL (Debug, Release) を行う
        - CMake で出先に指定した **CVPATH**\install\ 以下へインストールされる
- 使用s
    - Visutal Studio プロパティの指定
        - 追加のインクルードディレクトリ
            - **CVPATH**\install\include
        - 追加のライブラリディレクトリ (OpenCV を Staticライブラリでビルドした場合)
            - **CVPATH**\install\x64\vc17\staticlib
        - **CVPATH**\install\x64\vc17\staticlib\以下にある 全ての lib を追加する
            ~~~
            #ifdef _DEBUG
            #pragma comment(lib, "aded.lib")
            #pragma comment(lib, "IlmImfd.lib")
            #pragma comment(lib, "ippicvmt.lib") //!< これだけ d が作られなかった?
            #pragma comment(lib, "ippiwd.lib")
            #pragma comment(lib, "ittnotifyd.lib")
            #pragma comment(lib, "libjpeg-turbod.lib")
            #pragma comment(lib, "libopenjp2d.lib")
            #pragma comment(lib, "libpngd.lib")
            #pragma comment(lib, "libprotobufd.lib")
            #pragma comment(lib, "libtiffd.lib")
            #pragma comment(lib, "libwebpd.lib")
            #pragma comment(lib, "opencv_img_hash4100d.lib")
            #pragma comment(lib, "opencv_world4100d.lib")
            #pragma comment(lib, "zlibd.lib")
            #else
            #pragma comment(lib, "ade.lib")
            #pragma comment(lib, "IlmImf.lib")
            #pragma comment(lib, "ippicvmt.lib")
            #pragma comment(lib, "ippiw.lib")
            #pragma comment(lib, "ittnotify.lib")
            #pragma comment(lib, "libjpeg-turbo.lib")
            #pragma comment(lib, "libopenjp2.lib")
            #pragma comment(lib, "libpng.lib")
            #pragma comment(lib, "libprotobuf.lib")
            #pragma comment(lib, "libtiff.lib")
            #pragma comment(lib, "libwebp.lib")
            #pragma comment(lib, "opencv_img_hash4100.lib")
            #pragma comment(lib, "opencv_world4100.lib")
            #pragma comment(lib, "zlib.lib")
            #endif
            ~~~
    - CUDA 使用時
        - 追加のライブラリディレクトリ
            - $(CUDA_PATH)\lib\x64
        - cudart_static.lib を追加する
            ~~~
            #pragma comment(lib, "cudart_static.lib")
            ~~~
    - OpenCV を static ビルドした場合
        - C/C++ - Code Generation - Runtime Libarry を /MT, /MTd にする必要がある

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
