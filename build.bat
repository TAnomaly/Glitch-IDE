@echo off
echo ========================================
echo Basit Text Editor - Derleme Scripti
echo ========================================
echo.

REM g++ varsa onu kullan
where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo g++ bulundu, derleniyor...
    g++ -o text_editor.exe text_editor.cpp
    if %ERRORLEVEL% EQU 0 (
        echo Derleme basarili!
        echo text_editor.exe olusturuldu.
        echo.
        echo Programi calistirmak icin herhangi bir tusa basin...
        pause >nul
        text_editor.exe
    ) else (
        echo Derleme hatasi!
        pause
    )
    goto :end
)

REM cl (Visual Studio) varsa onu kullan
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Visual Studio derleyicisi bulundu, derleniyor...
    cl /EHsc text_editor.cpp
    if %ERRORLEVEL% EQU 0 (
        echo Derleme basarili!
        echo text_editor.exe olusturuldu.
        echo.
        echo Programi calistirmak icin herhangi bir tusa basin...
        pause >nul
        text_editor.exe
    ) else (
        echo Derleme hatasi!
        pause
    )
    goto :end
)

REM Derleyici bulunamadı
echo HATA: C++ derleyicisi bulunamadi!
echo.
echo Lutfen asagidakilerden birini yükleyin:
echo 1. MinGW/MSYS2 (g++ için)
echo 2. Visual Studio Build Tools (cl için)
echo 3. Dev-C++ veya Code::Blocks
echo.
pause

:end 