#include "pch.h"
#include "protocol-handler.h"

#include <vu>

BOOL reg_create_key(HKEY hKeyParent, PWCHAR subKey)
{
  HKEY  hKey;
  DWORD dwDisposition;
  DWORD ret = RegCreateKeyEx(hKeyParent, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
  if (ret == ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
  }

  return TRUE;
}

BOOL reg_write_value_string(HKEY hKeyParent, PWCHAR subKey, PWCHAR valueName, PWCHAR strData)
{
  HKEY hKey;
  DWORD ret = RegOpenKeyEx(hKeyParent, subKey, 0, KEY_WRITE, &hKey);
  if (ret == ERROR_SUCCESS)
  {
    ret = RegSetValueEx(hKey, valueName, 0, REG_SZ, LPBYTE(strData), ((lstrlen(strData) + 1) * 2));
  }

  if (ret == ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
  }

  return ret == ERROR_SUCCESS;
}

/**
 * ProtocolHandler
 */

ProtocolHandler::ProtocolHandler(const std::wstring& name) : m_name(name)
{
}

ProtocolHandler::~ProtocolHandler()
{
}

std::wstring ProtocolHandler::name(bool colon)
{
  return colon ? m_name + L":" : m_name;
}

bool ProtocolHandler::do_register()
{
  if (!vu::is_administrator()) // required elevation right ro register a protocol handler
  {
    return false;
  }

  // [HKEY_CLASSES_ROOT\<name>]
  // @="URL: <name> Protocol"
  // "URL Protocol"=""
  // 
  // [HKEY_CLASSES_ROOT\<name>\DefaultIcon]
  // @="path\\to\\<name>.exe,0"
  // 
  // [HKEY_CLASSES_ROOT\<name>\shell\open\command]
  // @="\"path\\to\\<name>.exe\" \"%1\""

  const auto file_path = vu::get_current_file_path_W();

  // [HKEY_CLASSES_ROOT\<name>]
  auto sub_key = m_name;
  auto val_str = vu::format_W(L"URL: %s Protocol", m_name.c_str());
  reg_create_key(HKEY_CLASSES_ROOT, PWCHAR(sub_key.c_str()));
  reg_write_value_string(HKEY_CLASSES_ROOT, PWCHAR(sub_key.c_str()), L"", PWCHAR(val_str.c_str()));
  reg_write_value_string(HKEY_CLASSES_ROOT, PWCHAR(sub_key.c_str()), L"URL Protocol", L"");

  // [HKEY_CLASSES_ROOT\<name>\DefaultIcon]
  sub_key = vu::format_W(L"%s\\DefaultIcon", m_name.c_str());
  val_str = vu::format_W(L"\"%s\",0", file_path.c_str());
  reg_create_key(HKEY_CLASSES_ROOT, PWCHAR(sub_key.c_str()));
  reg_write_value_string(HKEY_CLASSES_ROOT, PWCHAR(sub_key.c_str()), L"", PWCHAR(val_str.c_str()));

  // [HKEY_CLASSES_ROOT\<name>\shell\open\command]
  sub_key = vu::format_W(L"%s\\shell\\open\\command", m_name.c_str());
  val_str = vu::format_W(L"\"%s\" \"%%1\"", file_path.c_str());
  reg_create_key(HKEY_CLASSES_ROOT, PWCHAR(sub_key.c_str()));
  reg_write_value_string(HKEY_CLASSES_ROOT, PWCHAR(sub_key.c_str()), L"", PWCHAR(val_str.c_str()));

  return this->is_registered();
}

bool ProtocolHandler::is_registered()
{
  bool result = true;

  // [HKEY_CLASSES_ROOT\<name>]
  {
    vu::RegistryW reg(vu::registry_key::HKCR, m_name);
    result &= reg.key_exists();
  }

  // [HKEY_CLASSES_ROOT\<name>\shell\open\command]
  {
    auto sub_key = vu::format_W(L"%s\\shell\\open\\command", m_name.c_str());
    vu::RegistryW reg(vu::registry_key::HKCR, PWCHAR(sub_key.c_str()));
    result &= reg.key_exists();
  }

  return result;
}

bool ProtocolHandler::is_me(const std::wstring& cmd_line, bool only_check_containing)
{
  if (only_check_containing)
  {
    return vu::contains_string_W(cmd_line, this->name(true), true);
  }

  return vu::starts_with_W(cmd_line, this->name(true), true);
}
