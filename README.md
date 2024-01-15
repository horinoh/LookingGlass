
## ハードウエア
### Looking Glass Portrait
- USB-C, HDMI を PC に接続 (Connect USB-C and HDMI to PC)
- Portrait の電源を入れる
- スタート - 設定 - システム - ディスプレイ - マルチディスプレイ - 画面表示を拡張する - Looking Glass の画面を選択 - 拡大縮小とレイアウトを 100% にする (Start - Settings - System - Dsiplay - MultiDisplay - Extend these displays - Select Looking Glass Display - Scale and Layout - 100%) 

## 外部ライブラリ

### [LookingGlassCoreSDK](https://github.com/Looking-Glass/LookingGlassCoreSDK)
- LookingGlassCoreSDK\HoloPlayCore\dylib\Win64 を環境変数 Path に通しておく

### テクスチャ
#### DX
- [DirectXTK12](https://github.com/Microsoft/DirectXTK12) ライブラリ
    - DirectXTK_Desktop_2022_Win10.sln を開いてビルド
- [DirectXTex](https://github.com/microsoft/DirectXTex.git) ツール
    - DirectXTex_Desktop_2022.sln を開いてビルド
        - DDSView
        - Texassemble
        - Texconv
        - Texdiag
#### VK
- [gli](https://github.com/g-truc/gli) ライブラリ

### メッシュ
#### [FBX](https://aps.autodesk.com/developer/overview/fbx-sdk)
- SDK をインストールして、環境変数 FBX_SDK_PATH をインストール先に設定しておく


## アセット
- キルト画像(DDS)やメッシュ(FBX)は LookingGlass/Asset/ 以下へ配置しておく
    
### キルト画像
- [Jane_Guan_Space_Nap_qs8x6.webp](https://docs.lookingglassfactory.com/keyconcepts/quilts) を ペイントツール等で png 等の形式で保存してから dds へ変換する

### メッシュ
- [The Stanford 3D Scanning Repository](https://graphics.stanford.edu/data/3Dscanrep/)


