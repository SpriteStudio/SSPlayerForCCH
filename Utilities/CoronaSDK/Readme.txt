このフォルダが含むもの

lua2json
	Corona用のアニメーションluaファイルよりjsonへ変換を行うコンバータのサンプル
	です。

sapmle_load_lua_module
	.luaを動的に読み込むためのサンプルです。

sample_load_json
	lua2json で作成したjsonファイルの読み込みサンプルです。

いずれも元になる .lua ファイルは、モジュール化オプションを指定した状態で変換し
てください。
合わせて -n を指定し、アニメーション名のサフィックスを除外した方が効果的です。

変換例)
SsToCorona.exe -m -n *.ssax
