@echo off
setlocal
cd /d "%~dp0"

start "" "http://localhost:8000/WebView.html"
py -m http.server 8000