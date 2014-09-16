rem @echo off

if "%*" == "" (
	@echo SpriteStudio の.ssax ファイルを .js に変換し.html ファイルを開きます。
	@echo 入力： .ssax ファイル 画像ファイル
	@echo 一度に複数の .ssax を指定してはいけません
	@echo キャンバスのサイズはアニメーションのサイズに連動しません。
	@echo 変更が必要な場合、SsaView.html ファイル内の下部にある下記の場所を書き換えて下さい
	@echo canvas id="a_canvas" width="640" height="480"
	pause
	exit
)

setlocal

set _CD=%cd&
rem ドライブとパスをこのコンバータの場所に変更
%~d0
cd %~dp0

if not exist "tmp" mkdir "tmp"
cd tmp
del /Q *

rem データを手元にコピーしてリネーム。データ内の変数名を固定させるため
for %%F in (%*) do (
	xcopy %%F .\
)

ren *.ssax tmp.ssax

rem コンバート
..\..\..\..\Converter\bin\win\SsToHtml5 -i * -o PackedData > SsToHtml5.log
if ERRORLEVEL 1 (
	echo エラー: %ERRORLEVEL%
	pause
) else (
	rem ページをブラウザで開く
	cd ..
	start SsaView.html
)

cd %_CD%
