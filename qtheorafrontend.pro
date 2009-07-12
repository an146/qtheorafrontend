TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += fileinfo.h frontend.h transcoder.h qtimespinbox.h
FORMS += dialog.ui
SOURCES += fileinfo.cpp frontend.cpp main.cpp transcoder.cpp qtimespinbox.cpp
RESOURCES += resources.qrc
ICON += app.icns
RC_FILE += resources.rc

# Install
target.path = $$(PREFIX)/bin
INSTALLS += target

# Mac OS X deployment
macx {
	CONFIG+=x86 ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET += 10.4
	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
}
