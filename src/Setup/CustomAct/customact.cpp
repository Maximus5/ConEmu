#include <windows.h>
#include <shlobj.h>
#include <msiquery.h>
#include <rpc.h>

#include <comdef.h>

#include <string>
#include <list>
using namespace std;

#include "consize.hpp"

HMODULE g_h_module;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  g_h_module = hinstDLL;
  return TRUE;
}

#define CLEAN(type, object, code) \
  class Clean_##object { \
  private: \
    type& object; \
  public: \
    Clean_##object(type& object): object(object) { \
    } \
    ~Clean_##object() { \
      code; \
    } \
  }; \
  Clean_##object clean_##object(object);

struct Error
{
  int code;
  const wchar_t* message;
  const char* file;
  int line;
};

#define FAIL(_code) { \
  Error error; \
  error.code = _code; \
  error.message = NULL; \
  error.file = __FILE__; \
  error.line = __LINE__; \
  throw error; \
}

#define FAIL_MSG(_message) { \
  Error error; \
  error.code = 0; \
  error.message = _message; \
  error.file = __FILE__; \
  error.line = __LINE__; \
  throw error; \
}

#define CHECK_SYS(code) { if (!(code)) FAIL(HRESULT_FROM_WIN32(GetLastError())); }
#define CHECK_ADVSYS(code) { DWORD __ret = (code); if (__ret != ERROR_SUCCESS) FAIL(HRESULT_FROM_WIN32(__ret)); }
#define CHECK_COM(code) { HRESULT __ret = (code); if (FAILED(__ret)) FAIL(__ret); }
#define CHECK(code) { if (!(code)) FAIL_MSG(L#code); }

class NonCopyable
{
protected:
  NonCopyable() {}
  ~NonCopyable() {}
private:
  NonCopyable(const NonCopyable&);
  NonCopyable& operator=(const NonCopyable&);
};

template<typename Type> class Buffer: private NonCopyable
{
private:
  Type* buffer;
  size_t buf_size;
public:
  Buffer(): buffer(NULL), buf_size(0)
  {
  }
  Buffer(size_t size)
  {
    buffer = new Type[size];
    buf_size = size;
  }
  ~Buffer()
  {
    delete[] buffer;
  }
  void resize(size_t size)
  {
    if (buffer) delete[] buffer;
    buffer = new Type[size];
    buf_size = size;
  }
  Type* data()
  {
    return buffer;
  }
  size_t size() const
  {
    return buf_size;
  }
};

template<class CharType> basic_string<CharType> strip(const basic_string<CharType>& str)
{
  basic_string<CharType>::size_type hp = 0;
  basic_string<CharType>::size_type tp = str.size();
  while ((hp < str.size()) && ((static_cast<unsigned>(str[hp]) <= 0x20) || (str[hp] == 0x7F)))
    hp++;
  if (hp < str.size())
    while ((static_cast<unsigned>(str[tp - 1]) <= 0x20) || (str[tp - 1] == 0x7F))
      tp--;
  return str.substr(hp, tp - hp);
}

wstring widen(const string& str)
{
  return wstring(str.begin(), str.end());
}

wstring int_to_str(int val)
{
  wchar_t str[64];
  return _itow(val, str, 10);
}

wchar_t hex(unsigned char v)
{
  if (v >= 0 && v <= 9)
    return v + L'0';
  else
    return v - 10 + L'A';
}

#if 0            
wstring hex(unsigned char* data, size_t size)
{
  wstring result;
  result.reserve(size * 2);
  for (unsigned i = 0; i < size; i++)
  {
    unsigned char b = data[i];
    result += hex(b >> 4);
    result += hex(b & 0x0F);
  }
  return result;
}

unsigned char unhex(wchar_t v)
{
  if (v >= L'0' && v <= L'9')
    return v - L'0';
  else
    return v - L'A' + 10;
}

void unhex(const wstring& str, Buffer<unsigned char>& buf)
{
  buf.resize(str.size() / 2);
  for (unsigned i = 0; i < str.size(); i += 2)
  {
    unsigned char b = (unhex(str[i]) << 4) | unhex(str[i + 1]);
    buf.data()[i / 2] = b;
  }
}
#endif

wstring hex_str(unsigned val)
{
  wstring result(8, L'0');
  for (unsigned i = 0; i < 8; i++)
  {
    wchar_t c = hex(val & 0x0F);
    if (c) result[7 - i] = c;
    val >>= 4;
  }
  return result;
}

wstring get_system_message(HRESULT hr)
{
  wchar_t* sys_msg;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&sys_msg), 0, NULL);
  wstring message;
  if (len)
  {
    CLEAN(wchar_t*, sys_msg, LocalFree(static_cast<HLOCAL>(sys_msg)));
    message = strip(wstring(sys_msg));
  }
  else
  {
    message = L"HRESULT:";
  }
  message.append(L" (0x").append(hex_str(hr)).append(L")");
  return message;
}

#if 0
_COM_SMARTPTR_TYPEDEF(IShellLinkW, __uuidof(IShellLinkW));
_COM_SMARTPTR_TYPEDEF(IPersistFile, __uuidof(IPersistFile));
_COM_SMARTPTR_TYPEDEF(IShellLinkDataList, __uuidof(IShellLinkDataList));
#endif

#if 0
wstring get_shortcut_props(const wstring& file_name)
{
  IShellLinkWPtr sl;
  CHECK_COM(sl.CreateInstance(CLSID_ShellLink));

  IPersistFilePtr pf(sl);
  CHECK_COM(pf->Load(file_name.c_str(), STGM_READ));

  IShellLinkDataListPtr sldl(sl);
  DATABLOCK_HEADER* dbh;
  CHECK_COM(sldl->CopyDataBlock(NT_CONSOLE_PROPS_SIG, reinterpret_cast<VOID**>(&dbh)));
  CLEAN(DATABLOCK_HEADER*, dbh, LocalFree(dbh));

  return hex(reinterpret_cast<unsigned char*>(dbh), dbh->cbSize);
}
#endif

#if 0
void set_shortcut_props(const wstring& file_name, const wstring& props)
{
  IShellLinkWPtr sl;
  CHECK_COM(sl.CreateInstance(CLSID_ShellLink));

  IPersistFilePtr pf(sl);
  CHECK_COM(pf->Load(file_name.c_str(), STGM_READWRITE));
  
  Buffer<unsigned char> db;
  unhex(props, db);
  const DATABLOCK_HEADER* dbh = reinterpret_cast<const DATABLOCK_HEADER*>(db.data());

  IShellLinkDataListPtr sldl(sl);
  sldl->RemoveDataBlock(dbh->dwSignature);
  CHECK_COM(sldl->AddDataBlock(db.data()));

  CHECK_COM(pf->Save(file_name.c_str(), TRUE));
}
#endif

#if 0
void create_shortcut(const wstring& file_name, const wstring& target, const wstring& props)
{
  IShellLinkWPtr sl;
  CHECK_COM(sl.CreateInstance(CLSID_ShellLink));
  CHECK_COM(sl->SetPath(target.c_str()));
  CHECK_COM(sl->SetShowCmd(SW_SHOWMINNOACTIVE));

  Buffer<unsigned char> db;
  unhex(props, db);
  const DATABLOCK_HEADER* dbh = reinterpret_cast<const DATABLOCK_HEADER*>(db.data());
  IShellLinkDataListPtr sldl(sl);
  CHECK_COM(sldl->AddDataBlock(db.data()));

  IPersistFilePtr pf(sl);
  CHECK_COM(pf->Save(file_name.c_str(), TRUE));
}
#endif

wstring get_field(MSIHANDLE h_record, unsigned field_idx)
{
  DWORD size = 256;
  Buffer<wchar_t> buf(size);
  UINT res = MsiRecordGetStringW(h_record, field_idx, buf.data(), &size);
  if (res == ERROR_MORE_DATA)
  {
    size += 1;
    buf.resize(size);
    res = MsiRecordGetStringW(h_record, field_idx, buf.data(), &size);
  }
  CHECK_ADVSYS(res);
  return wstring(buf.data(), size);
}

#if 0
wstring get_property(MSIHANDLE h_install, const wstring& name)
{
  DWORD size = 256;
  Buffer<wchar_t> buf(size);
  UINT res = MsiGetPropertyW(h_install, name.c_str(), buf.data(), &size);
  if (res == ERROR_MORE_DATA)
  {
    size += 1;
    buf.resize(size);
    res = MsiGetPropertyW(h_install, name.c_str(), buf.data(), &size);
  }
  CHECK_ADVSYS(res);
  return wstring(buf.data(), size);
}
#endif

#if 0
wstring add_trailing_slash(const wstring& path)
{
  if ((path.size() == 0) || (path[path.size() - 1] == L'\\'))
  {
    return path;
  }
  else
  {
    return path + L'\\';
  }
}
#endif

bool is_installed(UINT (WINAPI *MsiGetState)(MSIHANDLE, LPCWSTR, INSTALLSTATE*, INSTALLSTATE*), MSIHANDLE h_install, const wstring& name)
{
  INSTALLSTATE st_inst, st_action;
  CHECK_ADVSYS(MsiGetState(h_install, name.c_str(), &st_inst, &st_action));
  INSTALLSTATE st = st_action;
  if (st <= 0) st = st_inst;
  if (st <= 0) return false;
  if ((st == INSTALLSTATE_REMOVED) || (st == INSTALLSTATE_ABSENT)) return false;
  return true;
}

bool is_component_installed(MSIHANDLE h_install, const wstring& component)
{
  return is_installed(MsiGetComponentStateW, h_install, component);
}

bool is_feature_installed(MSIHANDLE h_install, const wstring& feature)
{
  return is_installed(MsiGetFeatureStateW, h_install, feature);
}

#if 0
list<wstring> get_shortcut_list(MSIHANDLE h_install, const wchar_t* condition = NULL)
{
  list<wstring> result;
  PMSIHANDLE h_db = MsiGetActiveDatabase(h_install);
  CHECK(h_db);
  PMSIHANDLE h_view;
  wstring query = L"SELECT Shortcut, Directory_, Name, Component_ FROM Shortcut";
  if (condition)
    query.append(L" WHERE ").append(condition);
  CHECK_ADVSYS(MsiDatabaseOpenViewW(h_db, query.c_str(), &h_view));
  CHECK_ADVSYS(MsiViewExecute(h_view, 0));
  PMSIHANDLE h_record;
  while (true) {
    UINT res = MsiViewFetch(h_view, &h_record);
    if (res == ERROR_NO_MORE_ITEMS) break;
    CHECK_ADVSYS(res);

    wstring component = get_field(h_record, 4);

    if (is_component_installed(h_install, component)) {
      wstring shortcut_id = get_field(h_record, 1);
      wstring directory_id = get_field(h_record, 2);
      wstring file_name = get_field(h_record, 3);
      size_t pos = file_name.find(L'|');
      if (pos != wstring::npos)
        file_name.erase(0, pos + 1);
      wstring directory_path = get_property(h_install, directory_id);
      wstring full_path = add_trailing_slash(directory_path) + file_name + L".lnk";
      result.push_back(full_path);
    }
  }
  return result;
}
#endif

#if 0
void init_progress(MSIHANDLE h_install, const wstring& action_name, const wstring& action_descr, unsigned max_progress)
{
  PMSIHANDLE h_progress_rec = MsiCreateRecord(4);
  MsiRecordSetInteger(h_progress_rec, 1, 0);
  MsiRecordSetInteger(h_progress_rec, 2, max_progress);
  MsiRecordSetInteger(h_progress_rec, 3, 0);
  MsiRecordSetInteger(h_progress_rec, 4, 0);
  MsiProcessMessage(h_install, INSTALLMESSAGE_PROGRESS, h_progress_rec);
  MsiRecordSetInteger(h_progress_rec, 1, 1);
  MsiRecordSetInteger(h_progress_rec, 2, 1);
  MsiRecordSetInteger(h_progress_rec, 3, 1);
  MsiProcessMessage(h_install, INSTALLMESSAGE_PROGRESS, h_progress_rec);
  PMSIHANDLE h_action_start_rec = MsiCreateRecord(3);
  MsiRecordSetStringW(h_action_start_rec, 1, action_name.c_str());
  MsiRecordSetStringW(h_action_start_rec, 2, action_descr.c_str());
  MsiRecordSetStringW(h_action_start_rec, 3, L"[1]");
  MsiProcessMessage(h_install, INSTALLMESSAGE_ACTIONSTART, h_action_start_rec);
}
#endif

#if 0
void update_progress(MSIHANDLE h_install, const wstring& file_name)
{
  PMSIHANDLE h_action_rec = MsiCreateRecord(1);
  MsiRecordSetStringW(h_action_rec, 1, file_name.c_str());
  MsiProcessMessage(h_install, INSTALLMESSAGE_ACTIONDATA, h_action_rec);
}
#endif

wstring get_error_message(const Error& e)
{
  wstring str;
  str.append(L"Error at ").append(widen(e.file)).append(L":").append(int_to_str(e.line)).append(L": ");
  if (e.code)
    str.append(get_system_message(e.code));
  if (e.message)
  {
    if (e.code)
      str.append(L": ");
    str.append(e.message);
  }
  return str;
}

wstring get_error_message(const exception& e)
{
  wstring str;
  str.append(widen(typeid(e).name())).append(L": ").append(widen(e.what()));
  return str;
}

wstring get_error_message(const _com_error& e)
{
  wstring str;
  str.append(L"_com_error: ").append(get_system_message(e.Error()));
  return str;
}

void log_message(MSIHANDLE h_install, const wstring& message)
{
  OutputDebugStringW((message + L'\n').c_str());
  PMSIHANDLE h_rec = MsiCreateRecord(0);
  MsiRecordSetStringW(h_rec, 0, message.c_str());
  MsiProcessMessage(h_install, INSTALLMESSAGE_INFO, h_rec);
}

#define BEGIN_ERROR_HANDLER \
  try { \
    try {

#define END_ERROR_HANDLER \
    } \
    catch (const Error& e) { \
      log_message(h_install, get_error_message(e)); \
    } \
    catch (const exception& e) { \
      log_message(h_install, get_error_message(e)); \
    } \
    catch (const _com_error& e) { \
      log_message(h_install, get_error_message(e)); \
    } \
    catch (...) { \
      log_message(h_install, L"unknown exception"); \
    } \
  } \
  catch (...) { \
  }

#if 0
class ComInit: private NonCopyable
{
private:
  HRESULT hr;
public:
  ComInit()
  {
    hr = CoInitialize(NULL);
  }
  ~ComInit()
  {
    if (SUCCEEDED(hr))
      CoUninitialize();
  }
};
#endif

#if 0
class TempFile: private NonCopyable
{
private:
  wstring path;
public:
  TempFile(const wchar_t* ext)
  {
    UUID uuid;
    CHECK(UuidCreate(&uuid) == RPC_S_OK);
    RPC_WSTR uuid_str;
    CHECK(UuidToStringW(&uuid, &uuid_str) == RPC_S_OK);
    CLEAN(RPC_WSTR, uuid_str, RpcStringFreeW(&uuid_str));
    wstring unique_str(reinterpret_cast<const wchar_t*>(uuid_str));

    Buffer<wchar_t> buf(MAX_PATH);
    DWORD len = GetTempPathW(static_cast<DWORD>(buf.size()), buf.data());
    CHECK(len <= buf.size());
    CHECK_SYS(len);
    wstring temp_path = wstring(buf.data(), len);

    path = add_trailing_slash(temp_path) + unique_str + L"." + ext;
  }
  ~TempFile()
  {
    DeleteFileW(path.c_str());
  }
  wstring get_path() const
  {
    return path;
  }
};
#endif

#if 0
class File: private NonCopyable
{
protected:
  HANDLE h_file;
public:
  File(const wstring& file_path, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes)
  {
    h_file = CreateFileW(file_path.c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
    CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
  }
  ~File()
  {
    CloseHandle(h_file);
  }
  unsigned read(Buffer<char>& buffer)
  {
    DWORD size_read;
    CHECK_SYS(ReadFile(h_file, buffer.data(), static_cast<DWORD>(buffer.size()), &size_read, NULL));
    return size_read;
  }
  void write(const void* data, unsigned size)
  {
    DWORD size_written;
    CHECK_SYS(WriteFile(h_file, data, size, &size_written, NULL));
  }
};
#endif

#if 0
class Event: private NonCopyable
{
protected:
  HANDLE h_event;
public:
  Event(BOOL bManualReset, BOOL bInitialState)
  {
    h_event = CreateEvent(NULL, bManualReset, bInitialState, NULL);
    CHECK_SYS(h_event);
  }
  ~Event()
  {
    CloseHandle(h_event);
  }
  HANDLE handle()
  {
    return h_event;
  }
};
#endif

#if 0
class ServerPipe: private NonCopyable
{
protected:
  HANDLE h_pipe;
public:
  ServerPipe(LPCSTR lpName)
  {
    h_pipe = CreateNamedPipe(lpName, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 1, 0, 0, 0, NULL);
    CHECK_SYS(h_pipe != INVALID_HANDLE_VALUE);
  }
  ~ServerPipe()
  {
    CloseHandle(h_pipe);
  }
  void connect(OVERLAPPED& ov, DWORD timeout)
  {
    if (!ConnectNamedPipe(h_pipe, &ov)) {
      if (GetLastError() != ERROR_PIPE_CONNECTED)
      {
        CHECK_SYS(GetLastError() == ERROR_IO_PENDING);
        DWORD wait = WaitForSingleObject(ov.hEvent, timeout);
        CHECK_SYS(wait != WAIT_FAILED);
        CHECK(wait == WAIT_OBJECT_0);
      }
    }
  }
  void read(LPVOID buffer, DWORD size, OVERLAPPED& ov, DWORD timeout)
  {
    if (!ReadFile(h_pipe, buffer, size, NULL, &ov))
    {
      CHECK_SYS(GetLastError() == ERROR_IO_PENDING);
      DWORD wait = WaitForSingleObject(ov.hEvent, timeout);
      CHECK_SYS(wait != WAIT_FAILED);
      CHECK(wait == WAIT_OBJECT_0);
    }
    DWORD nread;
    CHECK_SYS(GetOverlappedResult(h_pipe, &ov, &nread, TRUE));
    CHECK(nread == size);
  }
};
#endif

#if 0
const wchar_t* c_lucida_shortcut_props = L"CC000000020000A00700F5005000190050001900FCFFFCFF00000000000000000000100036000000900100004C0075006300690064006100200043006F006E0073006F006C0065000000240034879F7C080000001C4100009CEA060356B7A87CF4BD240000000000F0BD2400190000000000000000000000010000000000000032000000040000000000000000000000000080000080000000808000800000008000800080800000C0C0C000808080000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00";
const wchar_t* c_consolas_shortcut_props = L"CC000000020000A00700F5005000190050001900FCFFFCFF000000000000000000001400360000009001000043006F006E0073006F006C006100730000006E0073006F006C0065000000240034879F7C080000001C4100009CEA060356B7A87CF4BD240000000000F0BD2400190000000000000000000000010000000000000032000000040000000000000000000000000080000080000000808000800000008000800080800000C0C0C000808080000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00";

const DWORD c_timeout = 5000;
#endif

#if 0
COORD get_con_size(const wstring& default_shortcut_props)
{
  ServerPipe pipe(c_pipe_name);
  Event ov_event(TRUE, FALSE);
  OVERLAPPED ov;
  memset(&ov, 0, sizeof(ov));
  ov.hEvent = ov_event.handle();

  HRSRC h_rsrc = FindResourceW(g_h_module, L"consize", L"exe");
  CHECK_SYS(h_rsrc);
  HGLOBAL res_data = LoadResource(g_h_module, h_rsrc);
  CHECK_SYS(res_data);
  LPVOID res_data_ptr = LockResource(res_data);
  CHECK_SYS(res_data_ptr);
  DWORD res_size = SizeofResource(g_h_module, h_rsrc);
  CHECK_SYS(res_size);
  TempFile tmp_exe(L"exe");
  {
    File file_exe(tmp_exe.get_path(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
    file_exe.write(res_data_ptr, res_size);
  }
  
  TempFile tmp_lnk(L"lnk");
  create_shortcut(tmp_lnk.get_path(), tmp_exe.get_path(), default_shortcut_props);

  SHELLEXECUTEINFOW sei;
  memset(&sei, 0, sizeof(sei));
  sei.cbSize = sizeof(sei);
  sei.fMask = SEE_MASK_FLAG_NO_UI;
  wstring file_path = tmp_lnk.get_path();
  sei.lpFile = file_path.c_str();
  sei.nShow = SW_HIDE;
  CHECK_SYS(ShellExecuteExW(&sei));

  pipe.connect(ov, c_timeout);

  DWORD process_id;
  pipe.read(&process_id, sizeof(process_id), ov, c_timeout);

  HANDLE h_process = OpenProcess(SYNCHRONIZE, FALSE, process_id);

  COORD con_size;
  pipe.read(&con_size, sizeof(con_size), ov, c_timeout);

  if (h_process)
    WaitForSingleObject(h_process, c_timeout);

  return con_size;
}
#endif

#if 0
void save_shortcut_props(MSIHANDLE h_install)
{
  ComInit com_init;
  wstring data;
  list<wstring> shortcut_list = get_shortcut_list(h_install, L"Target = '[#Far.exe]'");
  init_progress(h_install, L"SaveShortcutProps", L"Saving shortcut properties", shortcut_list.size());
  list<wstring> props_list;
  bool has_empty_props = false;
  for (list<wstring>::const_iterator file_path = shortcut_list.begin(); file_path != shortcut_list.end(); file_path++) {
    update_progress(h_install, *file_path);
    wstring props;
    BEGIN_ERROR_HANDLER
    props = get_shortcut_props(*file_path);
    END_ERROR_HANDLER
    if (props.empty()) has_empty_props = true;
    props_list.push_back(props);
  }
  wstring default_props;
  if (has_empty_props) {
    default_props = MsiEvaluateCondition(h_install, "VersionNT >= 601") == MSICONDITION_TRUE ? c_consolas_shortcut_props : c_lucida_shortcut_props;
    COORD con_size = { 0, 0 };
    BEGIN_ERROR_HANDLER
    con_size = get_con_size(default_props);
    END_ERROR_HANDLER
    if (con_size.X) {
      Buffer<unsigned char> db;
      unhex(default_props, db);
      NT_CONSOLE_PROPS* ntcp = reinterpret_cast<NT_CONSOLE_PROPS*>(db.data());
      ntcp->dwScreenBufferSize = con_size;
      ntcp->dwWindowSize = con_size;
      default_props = hex(db.data(), db.size());
    }
  }
  list<wstring>::const_iterator props = props_list.begin();
  for (list<wstring>::const_iterator file_path = shortcut_list.begin(); file_path != shortcut_list.end(); file_path++, props++) {
    data.append(*file_path).append(L"\n");
    if (props->empty())
      data.append(default_props);
    else
      data.append(*props);
    data.append(L"\n");
  }
  CHECK_ADVSYS(MsiSetPropertyW(h_install, L"RestoreShortcutProps", data.c_str()));
}
#endif

list<wstring> split(const wstring& str, wchar_t sep)
{
  list<wstring> result;
  size_t begin = 0;
  while (begin < str.size())
  {
    size_t end = str.find(sep, begin);
    if (end == wstring::npos)
      end = str.size();
    wstring sub_str = str.substr(begin, end - begin);
    result.push_back(sub_str);
    begin = end + 1;
  }
  return result;
}

#if 0
void restore_shortcut_props(MSIHANDLE h_install)
{
  ComInit com_init;
  wstring data = get_property(h_install, L"CustomActionData");
  list<wstring> str_list = split(data, L'\n');
  CHECK(str_list.size() % 2 == 0);
  init_progress(h_install, L"RestoreShortcutProps", L"Restoring shortcut properties", str_list.size());
  for (list<wstring>::const_iterator str = str_list.begin(); str != str_list.end(); str++) {
    BEGIN_ERROR_HANDLER
    const wstring& file_path = *str;
    str++;
    const wstring& props = *str;
    update_progress(h_install, file_path);
    set_shortcut_props(file_path, props);
    END_ERROR_HANDLER
  }
}
#endif

//+++
void update_feature_state(MSIHANDLE h_install)
{
  PMSIHANDLE h_db = MsiGetActiveDatabase(h_install);
  CHECK(h_db);
  PMSIHANDLE h_view;
  CHECK_ADVSYS(MsiDatabaseOpenView(h_db, "SELECT Feature FROM Feature WHERE Display = 0", &h_view));
  CHECK_ADVSYS(MsiViewExecute(h_view, 0));
  PMSIHANDLE h_record;
  while (true)
  {
    UINT res = MsiViewFetch(h_view, &h_record);
    if (res == ERROR_NO_MORE_ITEMS) break;
    CHECK_ADVSYS(res);

    wstring feature_id = get_field(h_record, 1);

    list<wstring> sub_features = split(feature_id, L'.');
    CHECK(sub_features.size() > 1);

    bool inst = true;
    for (list<wstring>::const_iterator feature = sub_features.begin(); feature != sub_features.end(); feature++) {
      if (!is_feature_installed(h_install, *feature)) {
        inst = false;
        break;
      }
    }
    CHECK_ADVSYS(MsiSetFeatureStateW(h_install, feature_id.c_str(), inst ? INSTALLSTATE_LOCAL : INSTALLSTATE_ABSENT));
  }
}

#if 0
void launch_shortcut(MSIHANDLE h_install)
{
  wstring launch_app = get_property(h_install, L"LAUNCHAPP");
  if (launch_app == L"1") { // launch Far shortcut
    list<wstring> shortcut_list = get_shortcut_list(h_install, L"Target = '[#Far.exe]'");
    CHECK(!shortcut_list.empty());
    launch_app = *shortcut_list.begin();
  }
  SHELLEXECUTEINFOW sei = { sizeof(SHELLEXECUTEINFOW) };
  sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
  sei.lpFile = launch_app.c_str();
  sei.nShow = SW_SHOWDEFAULT;
  CHECK_SYS(ShellExecuteExW(&sei));
}
#endif

//+++
// Find all hidden features with names like F1.F2...FN and set their state to installed
// when all the features F1, F2, ..., FN are installed.
// Example: feature Docs.Russian (Russian documentation) is installed only when feature Docs (Documentation)
// and feature Russian (Russian language support) are installed by user.
UINT __stdcall UpdateFeatureState(MSIHANDLE h_install)
{
  BEGIN_ERROR_HANDLER
  update_feature_state(h_install);
  return ERROR_SUCCESS;
  END_ERROR_HANDLER
  return ERROR_INSTALL_FAILURE;
}

#if 0
// Read console properties for all existing shortcuts and save them for later use by RestoreShortcutProps.
UINT __stdcall SaveShortcutProps(MSIHANDLE h_install)
{
  BEGIN_ERROR_HANDLER
  save_shortcut_props(h_install);
  return ERROR_SUCCESS;
  END_ERROR_HANDLER
  return ERROR_INSTALL_FAILURE;
}

// Read arguments from all existing shortcuts and save them for later use by RestoreShortcutProps2.
UINT __stdcall SaveShortcutProps2(MSIHANDLE h_install)
{
  BEGIN_ERROR_HANDLER
  save_shortcut_props(h_install);
  return ERROR_SUCCESS;
  END_ERROR_HANDLER
  return ERROR_INSTALL_FAILURE;
}

// Restore console properties saved by SaveShortcutProps.
// This action is run after shortcuts are recreated by upgrade/repair.
UINT __stdcall RestoreShortcutProps(MSIHANDLE h_install)
{
  BEGIN_ERROR_HANDLER
  restore_shortcut_props(h_install);
  return ERROR_SUCCESS;
  END_ERROR_HANDLER
  return ERROR_INSTALL_FAILURE;
}

// Restore arguments saved by SaveShortcutProps.
// This action is run after shortcuts are recreated by upgrade/repair.
UINT __stdcall RestoreShortcutProps2(MSIHANDLE h_install)
{
  BEGIN_ERROR_HANDLER
  restore_shortcut_props(h_install);
  return ERROR_SUCCESS;
  END_ERROR_HANDLER
  return ERROR_INSTALL_FAILURE;
}

// Launch Far via installed shortcut or command specified by LAUNCHAPP property
UINT __stdcall LaunchShortcut(MSIHANDLE h_install)
{
  BEGIN_ERROR_HANDLER
  launch_shortcut(h_install);
  return ERROR_SUCCESS;
  END_ERROR_HANDLER
  BEGIN_ERROR_HANDLER
  CHECK_ADVSYS(MsiDoAction(h_install, "LaunchApp"));
  return ERROR_SUCCESS;
  END_ERROR_HANDLER
  return ERROR_INSTALL_FAILURE;
}
#endif
