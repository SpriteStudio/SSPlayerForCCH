SSPlayerForCCH
==============

SSPlayer for Cocos2d-x/Corona SDK/HTML5

■フォルダ構成

SsPlayerConverter
  src
    common             共通コード
    converter          各コンバータのメインコード
  vc2010               VisualStudio2010用ソリューション
    lib                リンクが必要なboostライブラリ
    SsPlayerConverter  ソリューションフォルダ
  xcode4.6             Xcode4.6用ワークスペース
    lib                リンクが必要なboostライブラリ
    SsPlayerConverter  ワークスペースフォルダ




■VisualStudio2010のソリューション構築手順

●VSでvc2010に空のソリューションを作成
　（名前例：SsPlayerConverter）

　・ソリューションフォルダと同じ階層に必要なboostライブラリを置いておく（lib）

●各コンバータで共通のスタティックライブラリを作成
　（名前例：StaticLib）

　・ソリューションにsrc/common,src/converterを含むスタティックライブラリを作るプロジェクトを作成
　　新規プロジェクト > Visual C++ > Win32 > Win32プロジェクト > スタティックライブラリ
　　　※プリコンパイル済みヘッダーのチェックは外しました

　・作成したスタティックライブラリプロジェクトのプロパティを開き、以下の設定を変える
　　全て構成プロパティ、Debug,Release共に
　　　・全般>文字セット：マルチバイト文字セットを使用する
　　　・VC++ディレクトリ＞インクルードディレクトリ：追加する＞src/commonフォルダ、boostフォルダ

　・スタティックライブラリプロジェクトに以下の「既存の項目」を追加する
　　　・src/common下のファイル全部
　　　・src/converter下のファイル全部

　・ビルドしエラーがないことを確認する

●各コンバータのexeプロジェクト作成
　※以下、SsToHtml5を作成した場合

　・ソリューションに新規プロジェクトを作成
　　新規プロジェクト > Visual C++ > Win32 > Win32コンソールアプリケーション > コンソールアプリケーション
　　　※空のプロジェクトにして、プリコンパイル済みヘッダーのチェックは外しました

　・プロジェクトの依存関係を開きスタティックライブラリにチェックを入れる

　・作成したスタティックライブラリプロジェクトのプロパティを開き、以下の設定を変える
　　全て構成プロパティ、Debug,Release共に
　　　・全般>文字セット：マルチバイト文字セットを使用する

　・プロジェクトにmain.cppを追加（空のファイル）

　・main.cppのコードを以下のように。

　　　-----------------------------------------------------------------------------------
　　　#ifdef _DEBUG
　　　#pragma comment(lib, "../debug/StaticLib.lib")
　　　#pragma comment(lib, "../../lib/debug/libboost_filesystem-vc100-mt-gd-1_51.lib")
　　　#pragma comment(lib, "../../lib/debug/libboost_program_options-vc100-mt-gd-1_51.lib")
　　　#pragma comment(lib, "../../lib/debug/libboost_system-vc100-mt-gd-1_51.lib")
　　　#else
　　　#pragma comment(lib, "../release/StaticLib.lib")
　　　#pragma comment(lib, "../../lib/release/libboost_filesystem-vc100-mt-1_51.lib")
　　　#pragma comment(lib, "../../lib/release/libboost_program_options-vc100-mt-1_51.lib")
　　　#pragma comment(lib, "../../lib/release/libboost_system-vc100-mt-1_51.lib")
　　　#endif

　　　extern int Converter_SsToHtml5(int argc, const char *argv[]);

　　　int main(int argc, const char *argv[])
　　　{
　　　　　return Converter_SsToHtml5(argc, argv);
　　　}
　　　-----------------------------------------------------------------------------------

　・ビルドしエラーがないことを確認する




■Xcode4.6のワークスペース/プロジェクト構築手順

●Xcodeでワークスペースを作成

　・ワークスペースを保存するフォルダを作成しておく（名前例：SsPlayerConverter）

　・File > New > Workspace

　・ワークスペースと同じ階層に必要なboostライブラリを置いておく（lib）

●各コンバータで共通のスタティックライブラリを作成
　（名前例：StaticLib）

　・ワークスペースに新規プロジェクトを作成
　　New Project > OS X > Framework & Library > C/C++ Library
　　　・Product Name：StaticLib（例）
　　　・Company Identifier：jp.co.webtech.SsPlayerConverter（例）
　　　・Type：Static
　　　・Use Automatic Reference Counting：チェック外す

　・プロジェクトの以下の設定を変える
　　PROJECT > StaticLib > Build Settings
　　　・Base SDK：OS X 10.7
　　　・OS X Development Target：OS X 10.6
　　　・Header Search Paths：boostライブラリのフォルダ
　　　・C++ Standard Library：libstdc++(GNU C++ standard library)

　・スタティックライブラリに以下の項目を追加する
　　StaticLibを選んで > Add Files to "StaticLib"...
　　　・srcフォルダ（common,converterを含むフォルダ、中のファイルではなくフォルダ自体）

　・ビルドしエラーがないことを確認する

●各コンバータの実行ファイルプロジェクトを作成
　※以下、SsToHtml5を作成した場合

　・ワークスペースに新規プロジェクトを作成
　　New Project > OS X > Application > Command Line Tool
　　　・Product Name：SsToHtml5（例）
　　　・Company Identifier：jp.co.webtech.SsPlayerConverter（例）
　　　・Type：C++
　　　・Use Automatic Reference Counting：チェック外す

　・プロジェクトの以下の設定を変える
　　PROJECT > SsToHtml5 > Build Settings
　　　・Base SDK：OS X 10.7
　　　・OS X Development Target：OS X 10.6
　　　・C++ Standard Library：libstdc++(GNU C++ standard library)

　・参照するライブラリファイルを指定する
　　TARGET > SsToHtml5 > Build Phases > Link Binary With Libraries
　　　・libiconv.2.dylib
　　　・libStaticLib.a（スタティックライブラリプロジェクトで作成したライブラリ）
　　　・libboost_filesystem.a（boostライブラリ）
　　　・libboost_program_options.a（boostライブラリ）
　　　・libboost_system.a（boostライブラリ）

　・main.cppのコードを以下のように。

　　　-----------------------------------------------------------------------------------
　　　extern int Converter_SsToHtml5(int argc, const char *argv[]);

　　　int main(int argc, const char *argv[])
　　　{
　　　　　return Converter_SsToHtml5(argc, argv);
　　　}
　　　-----------------------------------------------------------------------------------

　・ビルドしエラーがないことを確認する
