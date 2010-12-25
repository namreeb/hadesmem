#include <iostream>

#pragma warning(push, 1)
#include <boost/proto/proto.hpp>
#include <boost/typeof/std/ostream.hpp>
#pragma warning(pop)

#include "HadesMem/Memory.h"

using namespace boost;

proto::terminal< std::ostream & >::type cout_ = { std::cout };

template< typename Expr >
void evaluate(Hades::Memory::MemoryMgr const& MyMem, Expr const & expr )
{
  MyMem;
  proto::default_context ctx;
  proto::eval(expr, ctx);
}

int main()
{
  Hades::Memory::MemoryMgr MyMem(L"vlc.exe");
  evaluate(MyMem, cout_ << "hello" << ',' << " world" );
  std::cin.get();
  return 0;
}
