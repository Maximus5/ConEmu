# Contributing to ConEmu

Want to contribute to ConEmu? Awesome, appreciated!

But please, be sure you are following some easy rules described below.

* [Reporting issues](#reporting-issues)
  * [Crash reports](#crash-reports)
  * [Issue attachments](#issue-attachments)
  * [Issue template](#issue-template)
* [Requesting features](#requesting-features)
* [Contributing with code](#contributing-with-code)
  * [Use latest branch](#use-latest-branch)
  * [Use separate commits](#use-separate-commits)
  * [Use descriptive commit messages](#use-descriptive-commit-messages)
  * [Don't use C++11 and C++14 features](#dont-use-c11-and-c14-features)



## Reporting issues

Appreciated, without proper users' feedback authors can't know about software problem.
But if you want to be helpful, if you want the problem to be fixed, you have to supply
authors with proper information.

Please read the [article on the official site](https://conemu.github.io/en/BadIssue.html)
how to report issues properly. Brief excerpts below.

* Please ensure that your problem does not belong to
  [third-party software](https://conemu.github.io/en/ThirdPartyProblems.html).

* Attach [descriptive screenshots](https://conemu.github.io/en/BadIssue.html#Screenshot)
  demonstating all steps you are doing. And it would be better, if you comment or highlight
  points of problems on screenshots.

* Share your [Settings](https://conemu.github.io/en/Settings.html).
  ConEmu has huge amount of options and, obviously, there are thousands of possible
  combinations! We can't guess, what you have configured to arise the problem!

* Also, additional infomation may be requsted from you **later**,
  such as [LogFiles](https://conemu.github.io/en/LogFiles.html)
  or [MemoryDumps](https://conemu.github.io/en/MemoryDump.html).


#### Crash reports

If you are reporting a crash, ConEmu shows you a thorough message
with information where [CrashDump](https://conemu.github.io/en/CrashDump.html)
was saved. Please, zip the file and upload it to
[DropBox](https://conemu.github.io/en/DropBox.html) or any other hosting.

#### Issue attachments

GitHub accepts only screenshots as direct attachments.
Please, don't try to cheat GitHub by changing file extensions,
or posting huge log files via comments body.

Zip your files, upload them to
[DropBox](https://conemu.github.io/en/DropBox.html)
or any other hosting (even to gist.github.com),
and post link in the issue.

#### Issue template

Don't omit required information! Versions are significant information!

~~~
### Versions
ConEmu build: ?????? x32/x64
OS version: Windows ??? x32/x64
Used shell version (Far Manager, git-bash, cmd, powershell, cygwin, whatever): ???

### Problem description

### Steps to reproduce
1. 
2. 
3. 

### Actual results

### Expected results

### Additional files
Settings, screenshots, logs, etc.
~~~



## Requesting features

* Update your installation first!
  [https://conemu.github.io/en/BadIssue.html#Update_your_installation]
  Don't waste your time, the feature may be already implemented!
* Search through [docs](https://conemu.github.io) and
  [Settings](https://conemu.github.io/en/Settings.html).
  You may overlook existing option.
* Still absent? Please, describe your suggestion in details.
  There are a lot of caricatures about software design.
  It would be a disappointment, if after implementation you would
  realize that it was not the feature you was waiting for.



## Contributing with code

You may read article [Source](https://conemu.github.io/en/Source.html)
in the ConEmu documentation. It contains latest actual information!

Also, before hacking on ConEmu you have to be sure you are using **newest** ConEmu build!
You problem or suggestion may be already fixed or implemented, so
[update your installation](https://conemu.github.io/en/BadIssue.html#Update_your_installation).

#### Use latest branch

Please, do not try to submit patches for old branches!
Patches for versions, which were obsolete months ago,
have absolutely no sense.

So, before hacking on ConEmu you have to be sure you are using
at least [master](https://github.com/ConEmu/tree/master)
or, even better, [daily](https://github.com/ConEmu/tree/daily)
branch.

Either clone ConEmu sources to new repository:

~~~
git clone --recursive https://github.com/Maximus5/ConEmu.git
~~~

Or pull/fetch latest commits from the
[official repository](https://conemu.github.io/en/Source.html).

~~~
git checkout master
git pull origin master
~~~

#### Use separate commits

Please, use separate commits for separate features or bugfixes!
Do not merge all your changes in one large commit, split them
in smaller chunks when possible!

Look at proper example below.

~~~
* f326d76 fix warning: there is possible null pointer dereference: pszNewTarget.
* 0408700 fix warning: there is possible null pointer dereference: pszValue.
~~~

#### Use descriptive commit messages

Please do not submit series of patches with same commit messages!
Describe either purpose of patch, or add affected file name at least.
It's hard to review list of identical commits!

Look at proper example below.

~~~
* 33d1973 Use helper functions and CEStr (ConsoleMain.cpp)
* 6ce49af Use helper functions and CEStr (TabBar.cpp)
* f326d76 fix warning: there is possible null pointer dereference: pszNewTarget.
* 0408700 fix warning: there is possible null pointer dereference: pszValue.
~~~

#### Don't use C++11 and C++14 features

ConEmu is supposed to be easily compiled with Visual Studio 2008,
to support OS Windows 2000 and ‘legacy’ computers.
