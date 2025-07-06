@echo off
echo ========================================
echo Vim-Like GUI Text Editor - Derleme
echo ========================================
echo.

REM g++ varsa onu kullan
where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo g++ bulundu, GUI versiyonu derleniyor...
    g++ -o vim_editor.exe vim_editor.cpp -lgdi32 -luser32 -lkernel32
    if %ERRORLEVEL% EQU 0 (
        echo Derleme basarili!
        echo vim_editor.exe olusturuldu.
        echo.
        echo GUI Text Editor baslatiliyor...
        vim_editor.exe
    ) else (
        echo Derleme hatasi!
        pause
    )
    goto :end
)

REM cl (Visual Studio) varsa onu kullan
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Visual Studio derleyicisi bulundu, GUI versiyonu derleniyor...
    cl /EHsc vim_editor.cpp user32.lib gdi32.lib
    if %ERRORLEVEL% EQU 0 (
        echo Derleme basarili!
        echo vim_editor.exe olusturuldu.
        echo.
        echo GUI Text Editor baslatiliyor...
        vim_editor.exe
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
echo.
pause

:end 