# LookingGlass

## アセット
- [キルト画像](https://docs.lookingglassfactory.com/keyconcepts/quilts)
<!--
- [デモダウンロード](https://docs.lookingglassfactory.com/getting-started/portrait/demo-holograms)
-->

### キルト画像
- [キルト画像](https://docs.lookingglassfactory.com/keyconcepts/quilts)
    - Jane_Guan_Space_Nap_qs8x6.webp を ペイントツール等で png 等の形式で保存してから dds へ変換する
    - LookingGlass/Asset/ フォルダ以下へ配置しておく
        - 別のキルト画像に変更したい場合は、**読み込むテクスチャの指定**と、**キルトグリッドの指定** (Column, Row) を変更する必要がある
        - QultDX::CreateTexture(), QuiltVK::CreateTexture() 内

## Looking Glass Portrait
- USB-C, HDMI を PC に接続 (Connect USB-C and HDMI to PC)
- Portrait の電源を入れる
- スタート - 設定 - システム - ディスプレイ - マルチディスプレイ - 画面表示を拡張する - Looking Glass の画面を選択 - 拡大縮小とレイアウトを 100% にする (Start - Settings - System - Dsiplay - MultiDisplay - Extend these displays - Select Looking Glass Display - Scale and Layout - 100%) 

## テクスチャ
### DX
- [DirectXTK12](https://github.com/Microsoft/DirectXTK12) ライブラリ
    - DirectXTK_Desktop_2022_Win10.sln を開いてビルド
- [DirectXTex](https://github.com/microsoft/DirectXTex.git) ツール
    - DirectXTex_Desktop_2022.sln を開いてビルド
        - DDSView
        - Texassemble
        - Texconv
        - Texdiag
### VK
- [gli](https://github.com/g-truc/gli) ライブラリ