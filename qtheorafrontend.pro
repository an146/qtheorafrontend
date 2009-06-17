TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += src

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build
RCC_DIR = build
QMAKE_LINK_OBJECT_SCRIPT = build/object_script

# Input
HEADERS += src/fileinfo.h src/frontend.h src/transcoder.h src/qtimespinbox.h src/util.h
FORMS += src/dialog.ui
SOURCES += src/fileinfo.cpp src/frontend.cpp src/main.cpp src/transcoder.cpp src/qtimespinbox.cpp src/util.cpp
RESOURCES += src/resources.qrc
ICON += src/app.icns
RC_FILE += src/resources.rc

# Install
target.path = $$(PREFIX)/bin
INSTALLS += target

# hack for cross-compilation on win32
win32 {
	QMAKE_LFLAGS += -Wl,-subsystem,windows
}

# Mac OS X deployment
macx {
	CONFIG+=x86 ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET += 10.4
	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
}
