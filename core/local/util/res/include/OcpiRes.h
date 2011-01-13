
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <OcpiOsDataTypes.h>
#include <cstdlib>
#include <memory>

namespace OCPI {
  namespace Util {
    typedef OCPI::OS::uint64_t ResAddrType;
    struct ResPool;
    class MemBlockMgr
    {
    public:
      MemBlockMgr(ResAddrType start, OCPI::OS::uint64_t size)
        throw( std::bad_alloc );
      ~MemBlockMgr()
        throw();
      int alloc( OCPI::OS::uint64_t nbytes, unsigned int alignment, ResAddrType& req_addr)
        throw( std::bad_alloc );
      int free(ResAddrType  addr )
        throw( std::bad_alloc );


    private:
      ResPool *m_pool;

    };
  }
}






