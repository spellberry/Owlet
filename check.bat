@echo off
IF EXIST "tidy" (
    echo Cleaning tidy folder
    rmdir "tidy" /s /q
)
mkdir tidy
FOR /F "tokens=*" %%G IN ('git diff --name-only HEAD') DO (
    echo %%G | findstr /R "^source\.* ^include\.*" >nul
    if not errorlevel 1 (
        echo Bzzz bzzz! Checking out %%G...
        clang-format -i "%%G" 
        clang-tidy "%%G" --export-fixes=tidy\%%~nG.yml >tidy\%%~nG.tidy
        
        if errorlevel 1 (
            echo What a mess! 
        ) else (
            echo All tidy!
        )
        echo Find the output in tidy\%%~nG.tidy and suggested fixes in tidy\%%~nG.yml
        echo Moving on to the next flower...
        echo.
    )
)