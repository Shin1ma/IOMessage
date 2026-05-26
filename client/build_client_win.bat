@echo OFF

cmake -B .\build\ -S ./ -DPLATFORM=WIN
set CL=/D_CRT_SECURE_NO_WARNINGS /DDEBUG_DO=1 /DREQUEST_FORMAT_DEBUG=1 /DCONFIG_DEBUG=1 /DCOMMUNICATOR_DEBUG=1 /DREQUEST_DEBUG=1
MSBuild.exe -target:rebuild -warnaserror .\build\IOMessage_client.sln