@rem Used to add source (cpp/hpp) files into VC projects
@rem TODO - makefiles support
@rem Usage: add-src Dummy.cpp Dummy.hpp
@rem Must(!) be called from the folder with VC project files

powershell -noprofile -command "& {%~dp0PrjEdit.ps1 %*}"
