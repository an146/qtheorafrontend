TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += frontend.h transcoder.h
FORMS += dialog.ui
SOURCES += frontend.cpp main.cpp transcoder.cpp
RESOURCES += resources.qrc
ICON += app.icns
RC_FILE += resources.rc

# Install
target.path = $$(PREFIX)/bin
INSTALLS += target
