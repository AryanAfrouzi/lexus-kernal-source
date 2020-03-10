#
#  makefile_module to make MODULE : version 3.00
#
#------------  <機能>  --------------------------------------------------------
#	<1> モジュールのビルド
#       - ソースからビルド
#         * 共有ライブラリジュール(lib*.so)
#         * OUTモジュール(*.out)
#         * 実行可能形式モジュール(*)
#
#       - バイナリ提供
#         * 共有ライブラリジュール(lib*.so)
#         * OUTモジュール(*.out)
#         * 実行可能形式モジュール(*)
#         * カーネルモジュール(*.ko)
#
#	<2> ライブラリのビルド
#       * インターフェイスライブラリ(lib*if.a)
#       * 静的ライブラリ(lib*.a)
#
#------------  <変更履歴>  ----------------------------------------------------
# 2010/08/18  0.123 by s.ogawa, copyright NEC corporation.
# 2010/08/25  0.20  by s.ogawa, NEC
# 2010/09/01  0.21  by s.ogawa, NEC
# 2010/09/03  0.22  STALIB/IFLIB .a/.so
# 2010/09/21  0.23  $(AR) instead of ar, for cross compile
# 2010/09/28  1.11  initial release.
# 2010/10/08  1.20  optimize each makefile, clean, make outfile
#                   make cleanMakefile
#                   bug: CHECKDEPS, re-make setup subdir, message
# 2010/10/26  1.21  add pginit in version script
#                   bug: always updated redefine header
# 2011/06/15  2.00  Support static library by FSI.
# 2011/09/19  2.01  1.余分なタブ文字を削除。
#                   2.testターゲットでLD_LIBRARY_PATH指定しないように修正。
#                   3.AR(アーカイブ)コマンド実行のライブラリ種別の判定を削除。
#                   4.out提供のモジュール対応
#                     local_outfile,clean_local_outfileターゲットを追加。
#                   5.静的ライブラリ提供のモジュール対応
#                     STALIBDIRSを定義を追加し,静的ライブラリPATH指定。
# 2011/09/20  2.10  公開されていないソースファイルが含まれている場合、
#                   オブジェクトファイルが登録されていればビルドできる様にした
#                   (存在しないソースディレクトリがあってもエラーとしない)
# 2011/11/01  2.20  共有ライブラリ(lib*.so)ビルド対応        by FSI koda
#                   1.SMODULEキーを追加
#                     SMODULE:共有ライブラリをビルドするサブモジュールの指定
#                   2.ターゲット"target_check"を追加
#                     以下のターゲット指定のチェックを行う
#                     (1)chkcpu(旧chkparmを修正):
#                        ・CPUアーキテクチャの指定をチェック
#                        ・makefile_$(TARGET_CPU).envの有無をチェック
#                     (2)chkkey(新規追加)       :
#                         実行ターゲットが正しく指定してあるかをチェックする
#                   3.その他の変更
#                     (1)ロジックの順序入れ替え :
#                        makefile.add等の外部ファイルを読み込んだ後に、
#                        TARGET_FILEの変数を指定する。
#                     (2)不要マクロ変数の削除   :
#                        ・VERSION
#                        ・SO_NAME
#                        ・LINK_NAME
#                     (3)各コメントの更新
# 2011/11/25  2.40  システムライブラリのリンク対応        by FSI koda
#                   1.システムの静的リンクライブラリのリンクサーチパスを追加
#                     SYS_STALIB_PATHをSTALIBDIRSへ追加する
#                   2.システムの共有ライブラリのリンクサーチパスを追加
#                     SYS_LIB_PATHをIFDIRSへ追加する
# 2011/12/12  2.50  共有ライブラリのビルド順序対応        by FSI koda
#                   共有ライブラリモジュール(so)のビルド順を通常モジュール(out)
#                   より先にビルドする。
#                   モジュールビルドを2段階に分け、共有ライブラリモジュールと
#                   通常モジュールとで、ビルドを区別する。
#                   区別するために、module_def.mkで指定した"OBJTYPE"キーと、
#                   ドメイン層で指定した"BUILDPHASE"を用いて、
#                   "BUILD_FLAG"(ビルドするか否か)を指定する。
# 2012/01/17  2.60  実行可能形式モジュールのビルド対応        by FSI koda
#                   1.実行形式モジュールのビルド対応 - section.4
#                     XMODULEキーが指定された場合、
#                     ビルド生成物は、TARGETキーの値とする。
#                   2.ビルド順序の対応 - section.6
#                     2.1.OBJTYPEに正しい値が設定されているか、
#                         確認し、正しくない場合はエラー表示する様にした。
#                     2.2.BUILDPHASEが指定されていなくても、
#                         ビルドできる様にした。
#                   3.TARGET_CPU指定の削除
#                     親makeの環境変数は、子makeに引き継がれるため、
#                     サブモジュールのmakefileに対して、
#                     TARGET_CPU指定する記述を削除。
#                   4.ドメイン間I/F共有ライブラリのリンク指定を削除
#                     IFLINKキーにデフォルトで指定されていた
#                     「libifout.so」オプションを削除する。
#                     リンクする場合は、LIBDEFキー(module_def.mk)に指定する。
# 2012/01/25  2.70  バイナリ提供モジュールのビルド対応        by FSI koda
#                   ロードモジュールのバイナリ提供だけでなく、
#                   共有ライブラリモジュールと実行可能形式モジュールの
#                   バイナリ提供にも対応する。
#                   1.各バイナリ提供ファイルの対応のため、新規ターゲットを追加
#                     1.1.ビルド時
#                         - local_sofile  : バイナリ提供ファイルのコピー
#                         - local_exefile : バイナリ提供ファイルのコピー
#                     1.2.クリーン時
#                         - clean_local_sofile  : バイナリ提供ファイルの削除
#                         - clean_local_exefile : バイナリ提供ファイルの削除
#                   2.バイナリ提供ファイルの有無の判定
#                     - do_build : SOとEXEファイルの有無の判定を追加
#                     - do_clean : 同上
#                   3.ターゲット名を変更
#                     【変更前】outfile
#                     【変更後】build_module
#                      モジュール全種のビルドを行うターゲットのため、名称変更
# 2012/03/22  2.80  Jenkinsビルド対応                       by FSI koda
#                   sysroot指定をしても-Lオプション指定に影響がないため、
#                   以下の処置を行って対応をした。
#                      システムの絶対パスをサーチする際は、
#                      SYS_LIB_PATH、および、SYS_STALIB_PATHの各要素の接頭部に
#                      Jenkins用のビルドルートを修飾する。
# 2012/04/12  2.90  実行時動的リンクのrpath対応
#                   ビルド生成物の格納先として下記を追加、生成物の削除にも対応する。
#                     - /home/naviwork/system/bin
#                     - /home/naviwork/system/lib
#                     - /home/naviwork/system/out
#                   /home/naviwork/system/ 以下の一括削除コマンドを追加
#                   /home/naviwork/system/ 以下に $(WORK_ROOT)/runtime/ 以下への
#                   シンボリックリンクを一括作成するコマンドを追加
#                   /home/naviwork/system/ 以下の環境のリセットコマンドを追加
# 2012/04/23  3.00  カーネルモジュール対応
#                   カーネルオブジェクト(*.ko)をバイナリ提供できるように
#                   既存のバイナリ提供モジュールの機能を拡張。
#                   以下の新規ターゲットを追加
#                     1.ビルド時
#                       - local_kofile        : カーネルオブジェクトのコピー
#                     2.クリーン時
#                       - clean_local_kofile  : カーネルオブジェクトの削除
#
# CAUTION! このファイルは直接編集しないでください。

SHELL = /bin/sh
MKDIR = mkdir -p	# make directory with make parent

ifeq (,$(TIMESTAMP))
    TIMEVAL =
else
    TIMEVAL = `date +"%S.%N"`
endif

ifeq (,$(shell $(SHELL) -c "echo -e"))
    ECHO	= echo -e $(TIMEVAL)
    ECHO_SIMPLE = echo -e
else
    ECHO	= echo $(TIMEVAL)
    ECHO_SIMPLE = echo
endif

export CHECKDEPS TIMESTAMP

# -----------------------------------------------------------------------------
# section.1 - module_def.mk    : モジュールのビルド設定の読み込み
# -----------------------------------------------------------------------------
-include module_def.mk

#OBJTOP = .
#とすると、コンパイル結果の.lo/.d/output.lstを、ドメインの区別なく
#obj直下のLMODULEディレクトリに配置することもできる。

OBJTOP = $(PUREDOMAIN)

# -----------------------------------------------------------------------------
# section.2 - directories
#
# WORK_ROOT         : common,runtime,ドメインの上位層
# PUREDOMAIN        : ドメイン(module_def.mkに定義)
# TARGET_CPU        : CPUアーキテクチャ(ビルド時に定義)
#                     makefile_XXX.envファイルを読み込む為の変数となる。
# COMMONDIR         : commonディレクトリ
#
# OBJDIR            : オブジェクトディレクトリ(クリーン時に削除される)
# LIBIFINDIR        : ドメイン内I/Fライブラリ格納ディレクトリ
# LIBIFINDIR        : ドメイン内I/Fライブラリ格納ディレクトリ
#
# SYS_LIB_PATH      : ユーザー指定のシステム共有ライブラリ格納ディレクトリ
#                     (module_def.mkに定義)
# SYS_STALIB_PATH   : ユーザー指定のシステム静的ライブラリ格納ディレクトリ
#                     (module_def.mkに定義)
# IFLIBDIRS         : SYS_LIB_PATHの定義。
#                     (他の共有ライブラリのパス指定は、makefile_xXX.envで定義)
# STALIBDIRS        : 静的ライブラリ格納ディレクトリ一覧
#
# RUNLIBDIR         : 共有ライブラリ格納ディレクトリ
# RUNBINDIR         : outファイル格納ディレクトリ
# RUNKERNELDIR      : カーネルオブジェクト格納ディレクトリ
#
# NAVIWORK_BIN_DIR  : /home/naviwork/system/bin
#                     EXEファイルのシンボリックリンク格納ディレクトリ
# NAVIWORK_LIB_DIR  : /home/naviwork/system/lib makefile_XXX.env で定義
#                     SOファイルのシンボリックリンク格納ディレクトリ
# NAVIWORK_OUT_DIR  : /home/naviwork/system/out makefile_XXX.env で定義
#                     OUTファイルのシンボリックリンク格納ディレクトリ
#
# -----------------------------------------------------------------------------
BUILDDIR     := $(CURDIR)

WORK_ROOT    := $(abspath $(CURDIR)/$(COMMONPATH))

COMMONDIR    := $(WORK_ROOT)/common
RUNDIR       := $(WORK_ROOT)/runtime

OBJDIR       := $(BUILDDIR)/obj/$(TARGET_CPU)/$(OBJTOP)

LIBIFINDIR   := $(WORK_ROOT)/$(PUREDOMAIN)/interface/libif/$(TARGET_CPU)
LIBIFOUTDIR  := $(COMMONDIR)/interface/libif/$(TARGET_CPU)

STAMODDIR    := $(BUILDDIR)/lib/$(TARGET_CPU)
STAINDIR     := $(WORK_ROOT)/$(PUREDOMAIN)/stalib/lib/$(TARGET_CPU)
STAOUTDIR    := $(COMMONDIR)/stalib/lib/$(TARGET_CPU)
STALIBDIRS   := $(STAMODDIR) $(STAINDIR) $(STAOUTDIR)

ifeq (, $(INTEGRATION_ENV))
    ####################################################
    #  for Local PC 
    ####################################################
    IFLIBDIRS    += $(SYS_LIB_PATH)
    STALIBDIRS   += $(SYS_STALIB_PATH)
else
    ####################################################
    #  for Jenkins Server
    ####################################################
    IFLIBDIRS    += $(foreach f, $(SYS_LIB_PATH), $(MEEGO_IMAGE_DIR)/$(f))
    STALIBDIRS   += $(foreach f, $(SYS_STALIB_PATH), $(MEEGO_IMAGE_DIR)/$(f))
endif

RUNLIBDIR    := $(RUNDIR)/lib/$(TARGET_CPU)
RUNBINDIR    := $(RUNDIR)/bin/$(TARGET_CPU)
RUNKERNELDIR := $(RUNDIR)/kernel/$(TARGET_CPU)

MAKEFILENAME := makefile

.SUFFIXES:
.SUFFIXES: .add .mk .env .awk .sh .lst .so .a .template

NAVIWORK_BIN_DIR := /home/naviwork/system/bin
NAVIWORK_LIB_DIR := /home/naviwork/system/lib
NAVIWORK_OUT_DIR := /home/naviwork/system/out

# -----------------------------------------------------------------------------
# section.3 - 外部ファイルの取り込み
#
# makefile_XXX.env  : 全体共通のビルド定義、全ビルドフェーズで用いる。
# cdef.mk           : マクロ定義
# makefile.add      : makefileの個別追加定義
# -----------------------------------------------------------------------------
-include $(WORK_ROOT)/makefile_$(TARGET_CPU).env
-include cdef.mk
-include makefile.add

# -----------------------------------------------------------------------------
# section.4 - 生成ファイルの定義
#
# OUTPUT          : コンパイルするファイルリスト
#
# LMODULE         : ロードモジュールのサブモジュールディレクトリ(module_def.mk)
# SMODULE         : 共有ライブラリのサブモジュールディレクトリ(同上)
# XMODULE         : 実行可能形式モジュールのサブモジュールディレクトリ(同上)
# MODULE_OUTPUT   : サブモジュール毎の生成ファイルリストのパス
#
# TARGET          : モジュール名 (module_def.mk)
# TARGET_NAME     : 生成ファイル名
# TARGET_FILE     : runtime側の生成ファイルのパス
# TARGET_SYSDIR   : システム側の生成ファイルのパス(シンボリックリンク作成先)
#
# MAPFILE         : マップファイル
#                   *.outファイル作成時に、outファイル間のシンボル衝突回避の為
#                   リンカスクリプトでglobalシンボルとlocalシンボルを記述
# MAP_GLOBAL      : マップファイルで定義するグローバル関数
#
# LOCAL_SO_TARGET : バイナリ提供の共有ライブラリモジュール(lib*.so)
# LOCAL_OUT_TARGET: バイナリ提供のロードモジュール(*.out)
# LOCAL_EXE_TARGET: バイナリ提供の実行可能形式モジュール(EXE形式)
# LOCAL_KO_TARGET : バイナリ提供のカーネルモジュール(*.ko)
# -----------------------------------------------------------------------------
OUTPUT = output.lst

ifneq ($(LMODULE),)
    MODULE_OUTPUT = $(LMODULE:%=$(OBJDIR)/$(TARGET)/%/$(OUTPUT))
    TARGET_NAME  ?= $(TARGET:%=%.out)
    TARGET_FILE  := $(TARGET_NAME:%=$(RUNBINDIR)/%)
    TARGET_SYSDIR := $(NAVIWORK_OUT_DIR)
endif

ifneq ($(SMODULE),)
    MODULE_OUTPUT = $(SMODULE:%=$(OBJDIR)/$(TARGET)/%/$(OUTPUT))
    TARGET_NAME  ?= $(TARGET:%=lib%.so)
    TARGET_FILE  := $(TARGET_NAME:%=$(RUNLIBDIR)/%)
    TARGET_SYSDIR := $(NAVIWORK_LIB_DIR)
endif

ifneq ($(XMODULE),)
    MODULE_OUTPUT = $(XMODULE:%=$(OBJDIR)/$(TARGET)/%/$(OUTPUT))
    TARGET_NAME  ?= $(TARGET)
    TARGET_FILE  := $(TARGET_NAME:%=$(RUNBINDIR)/%)
    TARGET_SYSDIR := $(NAVIWORK_BIN_DIR)
endif

ifeq (.out,$(suffix $(TARGET_NAME)))
    MAPFILE      ?= $(TARGET_NAME:%.out=$(OBJDIR)/$(TARGET)/%.map)
    MAP_GLOBAL   ?= $(TARGET)_main $(TARGET)_pginit
endif

LOCAL_SO_TARGET  := $(BUILDDIR)/bin/$(TARGET_CPU)/$(TARGET:%=lib%.so)
LOCAL_OUT_TARGET := $(BUILDDIR)/bin/$(TARGET_CPU)/$(TARGET:%=%.out)
LOCAL_EXE_TARGET := $(BUILDDIR)/bin/$(TARGET_CPU)/$(TARGET)
LOCAL_KO_TARGET  := $(BUILDDIR)/bin/$(TARGET_CPU)/$(TARGET:%=%.ko)

# -----------------------------------------------------------------------------
# section.5 - リンク定義、ライブラリ生成ファイルの定義
#
# LIBDEF        : リンクする共有ライブラリの定義(module_def.mkに定義)
# STALIBDEF     : リンクする静的ライブラリの定義(module_def.mkに定義)
# STAFLAG_ON    : 静的ライブラリのリンクフラグ定義の開始
# STAFLAG_OFF   : 静的ライブラリのリンクフラグ定義の終端
# IFLINK        : ビルド生成物へリンクする、共有ライブラリの定義
# STALINK       : ビルド生成物へリンクする、静的ライブラリの定義
#
# IFLIBIN       : ドメイン内I/Fライブラリのファイルリスト(module_def.mkに定義)
# IFLIBOUT      : ドメイン間I/Fライブラリのファイルリスト(module_def.mkに定義)
# STALIBIN      : ドメイン内静的ライブラリのファイルリスト(module_def.mkに定義)
# STALIBOUT     : ドメイン間静的ライブラリのファイルリスト(module_def.mkに定義)
# IFLIB*_FILE   : I/Fライブラリのファイルパスリスト
# STALIB*_FILE  : 静的ライブラリのファイルパスリスト
# -----------------------------------------------------------------------------
STAFLAG_ON      = -Wl,--whole-archive
STAFLAG_OFF     = -Wl,--no-whole-archive
IFLINK          = $(LIBDEF:%=-l%) $(IFLIBDIRS:%=-L%)
STALINK         = $(STAFLAG_ON) $(STALIBDEF:%=-l%) $(STALIBDIRS:%=-L%) $(STAFLAG_OFF)

LDSHARE        ?= -shared -Wl,-soname,

IFLIBIN_FILE   := $(IFLIBIN:%=$(LIBIFINDIR)/%.a)
IFLIBOUT_FILE  := $(IFLIBOUT:%=$(LIBIFOUTDIR)/%.a)
STALIBIN_FILE  := $(STALIBIN:%=$(STAINDIR)/%.a)
STALIBOUT_FILE := $(STALIBOUT:%=$(STAOUTDIR)/%.a)

LDFLAGS        += $(NAVIWORK_LIB_DIR:%=-L%) $(NAVIWORK_OUT_DIR:%=-L%)
LDFLAGS        += $(NAVIWORK_LIB_DIR:%=-Wl,-rpath %) $(NAVIWORK_OUT_DIR:%=-Wl,-rpath %)
LDFLAGS        += $(SYS_LIB_PATH:%=-Wl,-rpath %)

# -----------------------------------------------------------------------------
# section.6 - モジュールビルドのビルド判定
#             module_def.mkで指定してあるOBJTYPEと、makefile_domainで指定した
#             BUILDPHASEを照合し、ビルドするか判断する。
#              * OBJTYPEが正しい場合     : OBJTYPE_CHECK に "OK" を設定
#              * OBJTYPEが誤っている場合 : OBJTYPE_CHECK に "NG" を設定
#              * ビルドする  場合        : BUILD_FLAG    に "ON" を設定
#              * ビルドしない場合        : BUILD_FLAG    に "OFF"を設定
#
# OBJTYPE_CHECK : OBJTYPE に正しい値が設定されているか判断するフラグ
# BUILD_FLAG    : モジュールのビルドを行うか判断するフラグ
# OBJTYPE       : モジュールの種類(module_def.mkに定義)
# BUILDPHASE    : モジュールのビルド順序(makefile_domainにて指定)
# -----------------------------------------------------------------------------
# OBJTYPE の値を確認
ifneq ($(filter $(OBJTYPE),SO_MODULE OUT_MODULE EXE_MODULE), )
# OBJTYPEの値が正しい場合
    OBJTYPE_CHECK := OK
    ifeq ($(BUILDPHASE), )
    # ユーザー手動の場合: BUILDPHASEが指定されず、必ずビルドする
        BUILD_FLAG := ON
    else
    # インテグビルドの場合: BUILDPHASEが指定され、ビルドするか判断する
        ifeq ($(OBJTYPE), $(BUILDPHASE))
        # OBJTYPE と BUILDPHASE が一致する場合、ビルドする
            BUILD_FLAG := ON
        else
        # OBJTYPE と BUILDPHASE が一致しない場合、ビルドしない
            BUILD_FLAG := OFF
        endif
    endif
else
# OBJTYPEの値が誤っている場合
    OBJTYPE_CHECK := NG
endif

# -----------------------------------------------------------------------------
# section.7 - モジュールビルド
#
# all           : target_checkを行った上で、ビルドを行うのか、事前確認を行う
# do_build      : バイナリ提供ファイルのコピー、またはソースのビルドを行う
# target_check  : chkcpu, chkkey
# chkcpu        : CPUアーキテクチャ(TARGET_CPU)の指定をチェック
# chkkey        : ターゲットキー指定をチェック
# debug         : 各種変数の値を表示
# local_sofile  : バイナリ提供の共有ライブラリモジュール(lib*.so)をコピー
# local_outfile : バイナリ提供のロードモジュール(*.out)をコピー
# local_exefile : バイナリ提供の実行可能形式モジュール(EXE形式)をコピー
# local_kofile  : バイナリ提供のカーネルモジュール(*.ko)をコピー
# build_module  : 以下のいずれかのモジュールをビルド
#                 - 共有ライブラリモジュール(lib*.so)
#                 - ロードモジュール(*.out)
#                 - 実行可能形式モジュール(EXE形式)
# test          : outファイルのロードテスト
#                 step1 : "$(TARGET)_main"のシンボルをチェック
#                 step2 : linuxNaviを使用したロードテスト
# -----------------------------------------------------------------------------

.PHONY: force

.PHONY: all
all : target_check
	@if [ "$(OBJTYPE_CHECK)" = "OK" ] ;then \
		if [ "$(BUILD_FLAG)" = "ON" ] ;then \
			$(MAKE) do_build ;\
		elif [ "$(BUILD_FLAG)" = "OFF" ] ;then \
			$(ECHO) "Skipped : OBJTYPE[$(OBJTYPE)] is Unmatched to this buildphase[$(BUILDPHASE)]";\
		fi ;\
	elif [ "$(OBJTYPE_CHECK)" = "NG" ] ;then \
		$(ECHO) "Error : OBJTYPE is incorrect !";\
		exit 1 ;\
	fi

.PHONY: do_build
do_build : 
	@if [ ! -d $(NAVIWORK_BIN_DIR) ] ;then $(MKDIR) $(NAVIWORK_BIN_DIR) ;fi	;\
	if [ ! -d $(NAVIWORK_LIB_DIR) ] ;then  $(MKDIR) $(NAVIWORK_LIB_DIR) ;fi	;\
	if [ ! -d $(NAVIWORK_OUT_DIR) ] ;then  $(MKDIR) $(NAVIWORK_OUT_DIR) ;fi	;\
	if [ -f $(LOCAL_SO_TARGET) ] ;then    $(MAKE) local_sofile ;\
	elif [ -f $(LOCAL_OUT_TARGET) ] ;then $(MAKE) local_outfile ;\
	elif [ -f $(LOCAL_EXE_TARGET) ] ;then $(MAKE) local_exefile ;\
	elif [ -f $(LOCAL_KO_TARGET) ] ;then  $(MAKE) local_kofile ;\
	else \
		$(MAKE) build_module ;\
	fi

.PHONY: target_check chkcpu chkkey
target_check : chkcpu chkkey

chkcpu :
    ifndef TARGET_CPU
		@$(ECHO) "Error : Not specified TARGET_CPU !"	;\
		exit 1
    else
		@if [ ! -e $(WORK_ROOT)/makefile_$(TARGET_CPU).env ] ;then \
			$(ECHO) "Error : Not found makefile_$(TARGET_CPU).env !" ;\
			exit 1;\
		fi
    endif

chkkey :
        #TARGET-KEYが複数指定されている場合、エラーとする。
	@KEYCOUNT=0	;\
	[ -n "$(SMODULE)"    ] && KEYCOUNT=`expr $$KEYCOUNT + 1` \
	                       && $(ECHO) "TARGET-KEY(SMODULE) : $(SMODULE)" ;\
	[ -n "$(LMODULE)"    ] && KEYCOUNT=`expr $$KEYCOUNT + 1` \
	                       && $(ECHO) "TARGET-KEY(LMODULE) : $(LMODULE)" ;\
	[ -n "$(XMODULE)"    ] && KEYCOUNT=`expr $$KEYCOUNT + 1` \
	                       && $(ECHO) "TARGET-KEY(XMODULE) : $(XMODULE)" ;\
	[ -n "$(IFLIBIN)"    ] && KEYCOUNT=`expr $$KEYCOUNT + 1` \
	                       && $(ECHO) "TARGET-KEY(IFLIBIN) : $(IFLIBIN)" ;\
	[ -n "$(IFLIBOUT)"   ] && KEYCOUNT=`expr $$KEYCOUNT + 1` \
	                       && $(ECHO) "TARGET-KEY(IFLIBOUT) : $(IFLIBOUT)" ;\
	[ -n "$(STALIBIN)"   ] && KEYCOUNT=`expr $$KEYCOUNT + 1` \
	                       && $(ECHO) "TARGET-KEY(STALIBIN) : $(STALIBIN)" ;\
	[ -n "$(STALIBOUT)"  ] && KEYCOUNT=`expr $$KEYCOUNT + 1` \
	                       && $(ECHO) "TARGET-KEY(STALIBOUT) : $(STALIBOUT)" ;\
	if [ $$KEYCOUNT -gt 1 ] ; then	\
		$(ECHO) "TARGET-KEY = $$KEYCOUNT" ;\
		$(ECHO) "Error : Duplicated TARGET-KEY in module_def.mk !" ;\
		exit 1 ;\
	fi	;\

.PHONY: debug
debug :
	@echo "COMMONDIR=$(COMMONDIR)"
	@echo "BUILDDIR=$(BUILDDIR)"
	@echo "OBJDIR=$(OBJDIR)"
	@echo "RUNBINDIR=$(RUNBINDIR)"
	@echo "STALIBDIRS=$(STALIBDIRS)"
	@echo "LDFLAGS=$(LDFLAGS)"
	@echo "IFLINK=$(IFLINK)"
	@echo "STALINK=$(STALINK)"
	@echo "TARGET_FILE=$(TARGET_FILE)"
	@echo "MAPFILE=$(MAPFILE)"
	@echo "LMODULE=$(LMODULE)"
	@echo "SMODULE=$(SMODULE)"
	@echo "XMODULE=$(XMODULE)"
	@echo "MODULE_OUTPUT=$(MODULE_OUTPUT)"
	@echo "IFLIBIN_FILE=$(IFLIBIN_FILE)"
	@echo "IFLIBOUT_FILE=$(IFLIBOUT_FILE)"
	@echo "STALIBIN_FILE=$(STALIBIN_FILE)"
	@echo "STALIBOUT_FILE=$(STALIBOUT_FILE)"
	@echo "OUT_FILES={$(OUT_FILES)}"
	@echo "SUBMAKEDIR=$(SUBMAKEDIR)"

.PHONY: local_sofile
local_sofile :
	@[ -d $(RUNLIBDIR) ] || \
		( $(MKDIR) $(RUNLIBDIR) && $(ECHO) ..mkdir $(RUNLIBDIR))
	@cp $(LOCAL_SO_TARGET) $(RUNLIBDIR) && \
	ln -fs $(RUNLIBDIR)/$(TARGET:%=lib%.so) $(NAVIWORK_LIB_DIR) && \
	$(ECHO) "$(TARGET:%=lib%.so):***SO FILE COMPLETE***\n"

.PHONY: local_outfile
local_outfile :
	@[ -d $(RUNBINDIR) ] || \
		( $(MKDIR) $(RUNBINDIR) && $(ECHO) ..mkdir $(RUNBINDIR))
	@cp $(LOCAL_OUT_TARGET) $(RUNBINDIR) && \
	ln -fs $(RUNBINDIR)/$(TARGET:%=%.out) $(NAVIWORK_OUT_DIR) && \
	$(ECHO) "$(TARGET:%=%.out):***OUT FILE COMPLETE***\n"

.PHONY: local_exefile
local_exefile :
	@[ -d $(RUNBINDIR) ] || \
		( $(MKDIR) $(RUNBINDIR) && $(ECHO) ..mkdir $(RUNBINDIR))
	@cp $(LOCAL_EXE_TARGET) $(RUNBINDIR) && \
	ln -fs $(RUNBINDIR)/$(TARGET) $(NAVIWORK_BIN_DIR) && \
	$(ECHO) "$(TARGET):***EXE FILE COMPLETE***\n"

.PHONY: local_kofile
local_kofile :
	@[ -d $(RUNKERNELDIR) ] || \
		( $(MKDIR) $(RUNKERNELDIR) && $(ECHO) ..mkdir $(RUNKERNELDIR))
	@cp $(LOCAL_KO_TARGET) $(RUNKERNELDIR) && \
	$(ECHO) "$(TARGET:%=%.ko):***KO FILE COMPLETE***\n"

.PHONY: build_module
build_module : $(MAPFILE) $(TARGET_FILE)

$(MAPFILE) :
	@[ -d $(@D) ] || ( $(MKDIR) $(@D) && $(ECHO) ..mkdir $(@D))
	@$(ECHO_SIMPLE) "{\nglobal: $(MAP_GLOBAL:%=%;)\nlocal:\t*;\n};" | \
		sed "s/ / \t/g" | tr " " "\n" >$@

$(TARGET_FILE) : $(MODULE_OUTPUT)

	@[ -d $(@D) ] || ( $(MKDIR) $(@D) && $(ECHO) ..mkdir $(@D))

	@outList="$(foreach f, $^, `cat $(f)`)"				;\
	\
        #	check duplicate object	\
	\
	dupList=`echo "$$outList" | sed 's/ /\n/g' | sed '/^$$/d' |	\
		sort | uniq -D`	;\
	if [ -n "$$dupList" ]; then	\
		$(ECHO) "..duplicate object :\n$$dupList"	;\
		exit 1	;\
	fi		;\
	\
        #	link ojects	;\
	\
	$(ECHO) "..update $(@F) with" `echo "$$outList" | wc -w` "items."	;\
	curdir=`pwd`			;\
	cd $(OBJDIR)			;\
	[ -z "$(filter %.so %.out,$(TARGET_NAME))" ] || \
		ldshare="$(LDSHARE)$(TARGET_NAME)"	;\
	[ -z "$(MAPFILE)" ] || mapflag="-Wl,--version-script,$(MAPFILE)"	;\
	$(ECHO) "..$(CC) $(LDFLAGS)    $(STALINK)  $$ldshare $(IFLINK)    $$mapflag";\
	$(CC) $(LDFLAGS) $(STALINK) $$ldshare $(IFLINK) $$mapflag \
		-o $@ $$outList || exit 1	;\
	ln -fs $@ $(TARGET_SYSDIR)	;\
	cd $$curdir
	@$(ECHO) "$(@F):***$(OBJTYPE:%_MODULE=%) FILE COMPLETE***\n"

$(MODULE_OUTPUT) : force
	@srcdir=$(@:$(OBJDIR)/$(TARGET)/%/$(OUTPUT)=modules/%)	;\
	if [ -e $$srcdir ]; then \
		echo "BUILD : $$srcdir" ;\
		outdir=$(@:%/$(OUTPUT)=%)	;\
		$(MAKE) output -C $$srcdir -f $(MAKEFILENAME) \
			BUILDDIR=$(BUILDDIR) WORK_ROOT=$(WORK_ROOT) \
			OUTDIR=$$outdir || exit 1 ;\
	else \
		echo "SKIPPED : $$srcdir" ;\
	fi

# test build_module

.PHONY: test

ifeq (.out,$(suffix $(TARGET_NAME)))

test : target_check
	@[ -f $(TARGET_FILE) ] || exit 0	;\
	\
	$(ECHO) "\nstep.1 : main entry verification test."	;\
	\
	if nm $(TARGET_FILE) | grep "$(TARGET)_main" ; then	\
		$(ECHO) "... main entry is correct.\t[ OK ]"	;\
	else	$(ECHO) "$(@F): ..NOT FOUND:$(TARGET)_main. "	;\
		[ -n "$(TEST_CONTINUE)" ] || exit 1		;\
	fi	;\
	\
	if [ -f $(RUNBINDIR)/linuxNavi ]; then	\
		$(ECHO) "step.2 : load test."	;\
		\
		if $(RUNBINDIR)/linuxNavi loadtest $(TARGET_FILE) ; then	\
			$(ECHO) "... load test success.\t[ OK ]"	;\
		else	$(ECHO) "$(@F): .. load error."	;\
			[ -n "$(TEST_CONTINUE)" ] || exit 1	;\
		fi	;\
	fi
endif


# -----------------------------------------------------------------------------
# section.8 - ライブラリビルド
#
# lib           : iflib, stalib
# iflib         : IFライブラリ(lib*if.a)のビルド
# stalib        : Staticライブラリ(lib*.a)のビルド
# -----------------------------------------------------------------------------

.PHONY: iflib stalib lib

lib : iflib stalib

iflib : target_check $(IFLIBIN_FILE) $(IFLIBOUT_FILE)

stalib : target_check $(STALIBIN_FILE) $(STALIBOUT_FILE)

%.a : force
	@$(ECHO) "$(@F) start making..."
	@srcDirs="$(IFLIBIN_$(*F)) $(IFLIBOUT_$(*F)) $(STALIBIN_$(*F)) $(STALIBOUT_$(*F))";\
	\
	for src in $$srcDirs; do	\
		if [ -e modules/$$src ]; then \
			echo "BUILD : modules/$$src" ;\
			outdir=$(OBJDIR)/$(TARGET)/$$src	;\
			$(ECHO) "...src=$$src"	;\
			$(ECHO) "...outdir=$$outdir"	;\
			$(MAKE) output -C modules/$$src -f $(MAKEFILENAME)	\
			BUILDDIR=$(BUILDDIR) WORK_ROOT=$(WORK_ROOT) \
			OUTDIR=$$outdir || exit 1	;\
		else \
			echo "SKIPPED : modules/$$src" ;\
		fi \
	done	;\
	\
	[ -d $(@D) ] || ( $(MKDIR) $(@D) && $(ECHO) "..mkdir $(@D)")	;\
	\
        #	-- check output.lst is newer than target	\
	\
	build=""			;\
	outList=""			;\
	for src in $$srcDirs ; do	\
		output=$(OBJDIR)/$(TARGET)/$$src/$(OUTPUT)	;\
		[ -e $$output ] || continue			;\
		[ ! -e $@ -o $@ -ot $$output ] && build="yes"	;\
		outList="$$outList `cat $$output`"		;\
	done				;\
	[ -z "$$build" ] && $(ECHO)	\
		"$(@F):..already updated(`echo "$$srcDirs"|wc -w` components)\n" &&	\
		exit 0	;\
	\
        #	check duplicate object		\
	\
	dupList=`echo "$$outList" | sed 's/ /\n/g' | sed '/^$$/d' |	\
		sort | uniq -D`	;\
	if [ -n "$$dupList" ]; then	\
		$(ECHO) "..duplicate object :\n$$dupList"	;\
		exit 1	;\
	fi		;\
	\
        #	link objects			\
	\
	outCount=`echo "$$outList" | wc -w`	;\
	[ $$outCount -eq 0 ] && $(ECHO)	\
		"!$(@F):***no object found, makes no library\n" && exit 1	;\
	\
	$(ECHO) "..link with $$outCount items."	;\
	\
        #	-- link objects				\
	\
	cd $(OBJDIR)	;\
	$(ECHO) "..$(AR) rcs $@ $$outList"	;\
	$(AR) rcs $@ $$outList

	@$(ECHO) "$(@F):***LIBRARY COMPLETE***\n"

# -----------------------------------------------------------------------------
# section.9 - クリーン対象の定義
#
# ALL_LIB_MAKEFILE  : ライブラリのサブモジュールmakefile
# MODULE_MAKEFILE   : モジュールのサブモジュールmakefile
# ALL_MAKEFILE      : サブモジュールのmakefile
# SUBMAKEDIR        : サブモジュールのディレクトリ
# OUT_FILES         : 削除するファイル
# -----------------------------------------------------------------------------

ALLIFLIBIN       = $(foreach lib,$(IFLIBIN),$(IFLIBIN_$(lib)) )
ALLIFLIBOUT      = $(foreach lib,$(IFLIBOUT),$(IFLIBOUT_$(lib)) )
ALLSTALIBIN      = $(foreach lib,$(STALIBIN),$(STALIBIN_$(lib)) )
ALLSTALIBOUT     = $(foreach lib,$(STALIBOUT),$(STALIBOUT_$(lib)) )

ALL_LIB          = $(ALLIFLIBIN) $(ALLIFLIBOUT) $(ALLSTALIBIN) $(ALLSTALIBOUT)
ALL_LIB_MAKEFILE = $(ALL_LIB:%=modules/%/$(MAKEFILENAME))

ifneq ($(LMODULE),)
    MODULE_MAKEFILE  = $(LMODULE:%=modules/%/$(MAKEFILENAME))
endif
ifneq ($(SMODULE),)
    MODULE_MAKEFILE  = $(SMODULE:%=modules/%/$(MAKEFILENAME))
endif
ifneq ($(XMODULE),)
    MODULE_MAKEFILE  = $(XMODULE:%=modules/%/$(MAKEFILENAME))
endif

ALL_MAKEFILE     = $(sort $(MODULE_MAKEFILE) $(ALL_LIB_MAKEFILE) )
SUBMAKEDIR       = $(ALL_MAKEFILE:modules/%/$(MAKEFILENAME)=%)

OUT_FILES        = $(TARGET_FILE) $(IFLIBIN_FILE) $(IFLIBOUT_FILE) \
					$(STALIBIN_FILE) $(STALIBOUT_FILE) $(MAPFILE)


# -----------------------------------------------------------------------------
# section.10 - クリーン
#
# clean               : target_checkを行った上で、"CLEAN_CHECK"の値で分岐する
#                       定義されている場合   : precheck_clean
#                       定義されていない場合 : do_clean
# precheck_clean      : "OBJTYPE_CHECK" と "BUILD_FLAG"の値で
#                       クリーンを行うか事前確認する。
# do_clean            : バイナリ提供モジュールの削除、
#                       またはビルド生成物のクリーンを行う。
# clean_local_sofile  : バイナリ提供の共有ライブラリモジュール(lib*.so)を削除
# clean_local_outfile : バイナリ提供のロードモジュール(*.out)を削除
# clean_local_exefile : バイナリ提供の実行可能形式モジュール(EXE形式)を削除
# clean_local_kofile  : バイナリ提供の実行可能形式モジュール(*.ko)を削除
# cleansub_recursive  : サブモジュールのクリーン
# cleansub            : クリーン
# -----------------------------------------------------------------------------

.PHONY: clean precheck_clean do_clean 
.PHONY: clean_local_sofile clean_local_outfile clean_local_exefile clean_local_kofile
.PHONY: cleansub_recursive cleansub

clean: target_check
	@if [ "$(CHECK_CLEAN)" = "ON" ] ;then \
		$(MAKE) precheck_clean ;\
	else \
		$(MAKE) do_clean ;\
	fi

precheck_clean:
	@if [ "$(OBJTYPE_CHECK)" = "OK" ] ;then \
		if [ "$(BUILD_FLAG)" = "ON" ] ;then \
			$(MAKE) do_clean ;\
		elif [ "$(BUILD_FLAG)" = "OFF" ] ;then \
			$(ECHO) "Skipped : OBJTYPE[$(OBJTYPE)] is Unmatched to this buildphase[$(BUILDPHASE)]";\
		fi ;\
	elif [ "$(OBJTYPE_CHECK)" = "NG" ] ;then \
		$(ECHO) "Error : OBJTYPE is incorrect !";\
		exit 1 ;\
	fi

do_clean:
	@if [ -f $(LOCAL_SO_TARGET) ] ;then   $(MAKE) clean_local_sofile ;\
	elif [ -f $(LOCAL_OUT_TARGET) ] ;then $(MAKE) clean_local_outfile ;\
	elif [ -f $(LOCAL_EXE_TARGET) ] ;then $(MAKE) clean_local_exefile ;\
	elif [ -f $(LOCAL_KO_TARGET) ] ;then  $(MAKE) clean_local_kofile ;\
	else \
		$(MAKE) cleansub ;\
	fi

clean_local_sofile:
	@rm $(RUNLIBDIR)/$(TARGET:%=lib%.so) && \
	$(ECHO) "Removed : runtime/lib/$(TARGET_CPU)/$(TARGET:%=lib%.so)\n" || \
	$(ECHO) "Removed : $(TARGET:%=lib%.so) was already removed\n"
	@rm -f $(NAVIWORK_LIB_DIR)/$(TARGET:%=lib%.so) && \
	$(ECHO) "Removed : $(NAVIWORK_LIB_DIR)/$(TARGET:%=lib%.so)\n"

clean_local_outfile:
	@rm $(RUNBINDIR)/$(TARGET:%=%.out) && \
	$(ECHO) "Removed : runtime/bin/$(TARGET_CPU)/$(TARGET:%=%.out)\n" || \
	$(ECHO) "Removed : $(TARGET:%=%.out) was already removed\n"
	@rm -f $(NAVIWORK_OUT_DIR)/$(TARGET:%=%.out) && \
	$(ECHO) "Removed : $(NAVIWORK_OUT_DIR)/$(TARGET:%=%.out)\n"

clean_local_exefile:
	@rm $(RUNBINDIR)/$(TARGET) && \
	$(ECHO) "Removed : runtime/bin/$(TARGET_CPU)/$(TARGET)\n" || \
	$(ECHO) "Removed : $(TARGET) was already removed\n"
	@rm -f $(NAVIWORK_BIN_DIR)/$(TARGET) && \
	$(ECHO) "Removed : $(NAVIWORK_BIN_DIR)/$(TARGET)\n"

clean_local_kofile:
	@rm $(RUNKERNELDIR)/$(TARGET:%=%.ko) && \
	$(ECHO) "Removed : runtime/kernel/$(TARGET_CPU)/$(TARGET:%=%.ko)\n" || \
	$(ECHO) "Removed : $(TARGET:%=%.ko) was already removed\n"

cleansub_recursive:
	@for dir in $(SUBMAKEDIR) ; do	\
		if [ -f $$dir/$(MAKEFILENAME) ]; then	\
			outdir=$(OBJDIR)/$(TARGET)/$$dir	;\
			$(MAKE) $(@:%_recursive=%) -C modules/$$dir	\
				-f $(MAKEFILENAME)	\
				BUILDDIR=$(BUILDDIR) WORK_ROOT=$(WORK_ROOT)	\
				OUTDIR=$$outdir ;\
		fi	;\
	done

cleansub : cleansub_recursive
	@for delobj in $(OUT_FILES) $(sort $(dir $(OUT_FILES))); do	\
		\
		delobj="`echo $$delobj|sed 's@/\.@@'`"	;\
		\
		if [ -d $$delobj ] ; then	\
			rmdir -p $$delobj 2>/dev/null &&	\
			$(ECHO) "..rmdir $$delobj" ||:	;\
		elif [ -f $$delobj ] ; then	\
			rm $$delobj		;\
			$(ECHO) "..remove $$delobj"	;\
		fi			;\
	done

	@if [ ! -z $(TARGET_SYSDIR) ]; then	\
		delobj=`ls $(TARGET_SYSDIR)/$(TARGET_NAME) 2>/dev/nul`	;\
		if [ ! -z $$delobj ] ; then \
			rm -f $$delobj && $(ECHO) "Removed : $$delobj\n";\
		fi	;\
	fi

	@if [ -d $(OBJDIR)/$(TARGET) ]	; then	\
		$(ECHO) "..remain obj/$(OBJTOP)/$(TARGET), try to remove each member";\
		for dir in $(SUBMAKEDIR)	; do	\
			outdir=$(OBJDIR)/$(TARGET)/$$dir	;\
			$(MAKE) cleanDetail -C modules/$$dir	\
				-f $(MAKEFILENAME)	\
				BUILDDIR=$(BUILDDIR) WORK_ROOT=$(WORK_ROOT)	\
				OUTDIR=$$outdir ;\
		done	;\
	fi

