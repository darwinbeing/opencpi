


/*
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

/*
 * DMA transfer driver, which is built on the PIO XferServices.
 * The "hole" in the address space is a cheap way of having a
 * segmented address space with 2 regions...
 * FIXME: support endpoints with regions
 *
 * Endpoint format is:
 * <EPNAME>:<address>.<holeOffset>.<holeEnd>
 *
 * All values are hex.
 * If holeOffset is zero there is no hole
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include "KernelDriver.h"
#include "OcpiOsDebug.h"
#include "OcpiUtilMisc.h"
// We build this transfer driver using the PIO::XferServices
#include "DtPioXfer.h"


namespace OU = OCPI::Util;
namespace DT = DataTransfer;
namespace OCPI {
  namespace TILERA {

    class EndPoint;
    class XferFactory;

  }
}