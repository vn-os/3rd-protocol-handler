#pragma once

#include <string>

class ProtocolHandler
{
public:
  ProtocolHandler(const std::wstring& name);
  virtual ~ProtocolHandler();

  std::wstring name(bool colon = false);

  bool do_register();
  bool is_registered();
  bool is_me(const std::wstring& cmd_line, bool only_check_containing = false);

private:
  std::wstring m_name;
};
