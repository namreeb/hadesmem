// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

template <typename Proxy>
class ProxyCall
{
public:
  explicit ProxyCall(Proxy* proxy)
    : proxy_(proxy)
  {
    proxy_->PreCall();
  }

  ProxyCall(ProxyCall&& other)
    : proxy_(other.proxy_)
  {
    other.proxy_ = nullptr;
  }

  ProxyCall& operator=(ProxyCall&& other)
  {
    proxy_ = other.proxy_;
    other.proxy_ = nullptr;

    return *this;
  }

  ~ProxyCall()
  {
    if (proxy_)
    {
      proxy_->PostCall();
    }
  }

private:
  ProxyCall(ProxyCall const&);
  ProxyCall& operator=(ProxyCall const&);

  Proxy* proxy_;
};

template <typename Proxy>
ProxyCall<Proxy> MakeProxyCall(Proxy* proxy)
{
  return ProxyCall<Proxy>(proxy);
}
