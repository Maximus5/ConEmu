This is test tool, sort of ANSI x3.64 sequences debugger.
Run RunDebugger.cmd and type commands in the lower split.

Command line switches
  Run server:
    AnsiDbg.exe -srv PipeName
  Run client (control part):
    AnsiDbg.exe -in PipeName
  Run simple line reader:
    AnsiDbg.exe -read

Client (control part) hotkeys

  When Client is successfully connected to server, you may press some hotkeys
  to simplifying ANSI debugging (sending certain sequences)

  Up/Down/Left/Right  - Just move cursor.
  Home                - Move cursor to the first column of the current line.
  End                 - Move cursor to the last column of the current line,
                        this may be not correct column in server width differs.
  Ctrl+End            - Scroll buffer 9999 lines down (ready to ConEmu TrueColor).
  PgUp                - Scroll screen (whole buffer) up by lines. New lines are added at the bottom.
  PgDn                - Scroll screen (whole buffer) down by lines. New lines are added at the top.
  Del                 - Print ‘Del’ hint in the client part.
  Ctrl+Del            - Delete line, scroll below lines, insert line at the bottom.
  Alt+Del             - Delete char, scroll line reminder to the left.
  Shift+Del           - Clear char.
  Ins                 - Print ‘Ins’ hint in the client part.
  Ctrl+Ins            - Insert line,
  Alt+Ins             - Insert char.

  Alt+X               - Exit session.
  Alt+L               - Clear screen, goto {1,1} coord.
  Alt+F               - Print 25 lines of text.

  Alt+1               - Request terminal primary DA.
                        ^[[?1;2c
  Alt+2               - Request terminal secondary DA
                        ^[>67;140227;0c
  Alt+3               - Request terminal status
                        ^[[0n
  Alt+4               - Request cursor pos [row;col]
                        ^[[7;31R
  Alt+5               - Request text area size [height;width]
                        ^[[8;33;173t
  Alt+6               - Request screen area size [height;width]
                        ^[[8;33;173t
  Alt+7               - Request terminal title
                        ^[]l"AnsiDbg.exe" -srv TestAnsi^[\
