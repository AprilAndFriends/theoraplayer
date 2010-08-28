@echo off
echo "Creating Directory structure..."
mkdir bin
cd bin
md Debug
md Debug_Static
md Release
md Release_Static
cd ..
echo "Copying dlls to all target dirs"
copy lib\*.dll bin\Debug
copy lib\*.dll bin\Debug_Static
copy lib\*.dll bin\Release
copy lib\*.dll bin\Release_Static
