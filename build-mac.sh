#!/bin/sh
qmake -spec macx-g++
make

fpath=qtheorafrontend.app/Contents/Frameworks
exe=qtheorafrontend.app/Contents/MacOS/qtheorafrontend
rm -Rf $fpath
mkdir -p $fpath

exerelpath() {
	echo @executable_path/../Frameworks/$1.framework/Versions/4.0/$1
}

correct_link() {
	lib=$1
	exe=$2
	lpath=`otool -L $exe | grep $lib | awk '{print $1;}'`
	install_name_tool -change $lpath `exerelpath $lib` $exe
}

for lib in QtCore QtGui; do
	cp -R /Library/Frameworks/$lib.framework $fpath || {
		echo Could not find $lib
		exit 1
	}
	install_name_tool -id `exerelpath $lib` $fpath/$lib.framework/Versions/4.0/$lib
	correct_link $lib $exe
done
correct_link QtCore $fpath/QtGui.framework/Versions/4.0/QtGui

find $fpath -name '*debug*' -print0 | xargs -0 /bin/rm -Rf
find $fpath -name '*Headers*' -print0 | xargs -0 /bin/rm -Rf
#tar -zcf qtheorafrontend.app.tar.gz qtheorafrontend.app
