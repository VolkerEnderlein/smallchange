@echo off

echo Installing to %COINDIR%

if not "%COINDIR%"=="" goto coindirset
echo The COINDIR environment variable must be set to point to a directory
echo to be able to perform the installation procedure.
exit
:coindirset
if exist %COINDIR%\*.* goto coindirexists
echo The COINDIR environment variable must point to an existing directory
echo to be able to perform the installation procedure.
exit
:coindirexists

pushd %COINDIR%

REM **********************************************************************
REM * Create all the directories for the installed files

if exist bin\*.* goto binexists
echo mkdir %COINDIR%\bin
mkdir bin
:binexists
if exist lib\*.* goto libexists
echo mkdir %COINDIR%\lib
mkdir lib
:libexists
if exist include\*.* goto includeexists
echo mkdir %COINDIR%\include
mkdir include
:includeexists
chdir include
if exist SmallChange\*.* goto smallchangeexists
echo mkdir %COINDIR%\include\SmallChange
mkdir SmallChange
:smallchangeexists
chdir SmallChange
if exist actions\*.* goto actionsexists
echo mkdir %COINDIR%\include\SmallChange\actions
mkdir actions
:actionsexists
if exist draggers\*.* goto draggersexists
echo mkdir %COINDIR%\include\SmallChange\draggers
mkdir draggers
:draggersexists
if exist elements\*.* goto elementsexists
echo mkdir %COINDIR%\include\SmallChange\elements
mkdir elements
:elementsexists
if exist engines\*.* goto enginesexists
echo mkdir %COINDIR%\include\SmallChange\engines
mkdir engines
:enginesexists
if exist nodes\*.* goto nodesexists
echo mkdir %COINDIR%\include\SmallChange\nodes
mkdir nodes
:eventhandlersexists
if exist eventhandlers\*.* goto eventhandlersexists
echo mkdir %COINDIR%\include\SmallChange\eventhandlers
mkdir eventhandlers
:eventhandlersexists
if exist misc\*.* goto miscexists
echo mkdir %COINDIR%\include\SmallChange\misc
mkdir misc
:miscexists
if exist nodekits\*.* goto nodekitsexists
echo mkdir %COINDIR%\include\SmallChange\nodekits
mkdir nodekits
:nodekitsexists
if exist nodes\*.* goto nodesexists
echo mkdir %COINDIR%\include\SmallChange\nodes
mkdir nodes
:nodesexists

popd

REM **********************************************************************
REM * Copy files

xcopy *.dll %COINDIR%\bin\ /R /Y
xcopy Debug\*.pdb %COINDIR%\bin\ /R /Y
xcopy Debug\*.lib %COINDIR%\lib\ /R /Y
REM xcopy Release\*.pdb %COINDIR%\bin\ /R /Y
xcopy Release\*.lib %COINDIR%\lib\ /R /Y

REM this part needs to be autogenerated
echo Installing headers...
call installsmallchangeheaders.bat
echo Done.
