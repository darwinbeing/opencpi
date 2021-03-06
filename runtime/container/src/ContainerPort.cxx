
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

#include "OcpiOsAssert.h"
#include "OcpiUtilCDR.h"
#include "Container.h"
#include "ContainerWorker.h"
#include "ContainerPort.h"

namespace OCPI {
  namespace Container {
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OD = OCPI::DataTransport;
    using namespace OCPI::RDT;

    PortData::PortData(const OU::Port &mPort, bool isProvider, unsigned xferOptions,
		       const OU::PValue *params, PortConnectionDesc *desc)
      : m_ordinal(mPort.m_ordinal), m_isProvider(isProvider), m_connectionData(desc)
    {
      Descriptors &d = getData().data;
      d.type = isProvider ? ConsumerDescT : ProducerDescT;
      d.role = NoRole;
      d.options = xferOptions;
      bzero((void *)&d.desc, sizeof(d.desc));
      d.desc.nBuffers =
	(uint32_t)(DEFAULT_NBUFFERS > mPort.m_minBufferCount ?
		   DEFAULT_NBUFFERS : mPort.m_minBufferCount);
      if ((d.desc.dataBufferSize = (uint32_t)mPort.m_bufferSize))
	ocpiDebug("PortData %s(%p): setting buffer size from metadata: %zu",
		  mPort.m_name.c_str(), this, mPort.m_bufferSize);
      else {
	d.desc.dataBufferSize = DEFAULT_BUFFER_SIZE;
	ocpiDebug("PortData %s(%p): setting buffer size from default: %zu",
		  mPort.m_name.c_str(), this, DEFAULT_BUFFER_SIZE);
      }
      setPortParams(mPort, params);
    }
    // Set parameters for a port, whether at creation/construction time or later at connect time
    void PortData::setPortParams(const OU::Port &mPort, const OU::PValue *params) {
      OA::ULong ul;

      if (OU::findULong(params, "bufferCount", ul))
	if (ul < mPort.m_minBufferCount)
	  throw OU::Error("bufferCount is below worker's minimum");
        else
	  getData().data.desc.nBuffers = ul;
      if (OU::findULong(params, "bufferSize", ul))
	if (ul < mPort.m_minBufferSize)
	  throw OU::Error("bufferSize %u is below worker's minimum: %zu",
			  ul, mPort.m_minBufferSize);
        else {
	  getData().data.desc.dataBufferSize = ul;
	  ocpiDebug("Portdata %s(%p): setting buffer size from parameter: %zu",
		    mPort.m_name.c_str(), this, (size_t)ul);
	}
      
      const char *s;
      if (OU::findString(params, "xferRole", s)) {
	PortRole role;
	if (!strcasecmp(s, "passive"))
	  role = Passive;
	else if (!strcasecmp(s, "active") ||
		 !strcasecmp(s, "activemessage"))
	  role = ActiveMessage;
	else if (!strcasecmp(s, "flowcontrol") ||
		 !strcasecmp(s, "activeflowcontrol"))
	  role = ActiveFlowControl;
	else if (!strcasecmp(s, "activeonly"))
	  role = ActiveOnly;
	else
	  throw OU::Error("xferRole property must be passive|active|flowcontrol|activeonly");
	if (!(getData().data.options & (1 << role)))
	  throw OU::Error("xferRole of \"%s\" not supported by port \"%s\"",
			  s, mPort.m_name.c_str());
	getData().data.role = role;
	getData().data.options |= (1 << OCPI::RDT::MandatedRole);
      }
    }

    BasicPort::BasicPort(const OU::Port & metaData, bool isProvider, unsigned options,
			 OS::Mutex &mutex, const OU::PValue *params, PortConnectionDesc *desc)
      : PortData(metaData, isProvider, options, params, desc), OU::SelfRefMutex(mutex),
	myDesc(getData().data.desc), m_metaPort(metaData)
    {
    }

    BasicPort::~BasicPort(){}
    void BasicPort::startConnect(const Descriptors */*other*/, const OU::PValue *){} // default

    // Convert PValues into descriptor values, with metadata constraint checking
    void BasicPort::setConnectParams(const OU::PValue *params) {
      setPortParams(m_metaPort, params);
      // There are no connection parameters (yet) other than those that can be provided
      // to ports before connection.
    }

    // Do the work on this port when connection parameters are specified.
    // This is still prior to receiving info from the other side, thus this is not necessarily
    // the final info.
    // FIXME: we should error check against bitstream-fixed parameters
    void BasicPort::applyConnectParams(const Descriptors *other, const OU::PValue *params) {
      setConnectParams(params);
      startConnect(other, params);
    }

    // This base class constructor for generic initialization
    // FIXME: parse buffer count here at least? (check that others don't do it).
    Port::Port(Container &container, const OU::Port &mPort, bool isProvider,
	       unsigned xferOptions, const OU::PValue *params, PortConnectionDesc *desc) :
      BasicPort(mPort, isProvider, xferOptions, container, params, desc),
      m_container(container), m_canBeExternal(true)
    {
    }

    Container &Port::container() const { return m_container; }

    void Port::loopback(OA::Port &) {}

    bool Port::hasName(const char *name) {
      return (name == m_metaPort.m_name );
    }

    // The default behavior is that there is nothing special to do between
    // ports of like containers.
    bool Port::connectLike(Port &other, const OU::PValue *myProps,
			   const OU::PValue *otherProps) {
      (void)other;(void)myProps;(void)otherProps;
      return false;
    }

    // This funkiness is to ensure that connection-related (as opposed to port-related) parameters
    // end up on both lists.  FIXME make this automatic storage etc.
    // We assume these connection parameters are initially only in one place
    static void
    mergeConnectParams(const OU::PValue *otherParams,           // potential source of connect params
		       const char *preferred,                   // callers preferred transport
		       const OU::PValue *&toParams,             // params possibly augmented
		       OU::PValue *&newParams) {                // new list if needed
      newParams = NULL;
      const char *transport = NULL;
      if (!OU::findString(otherParams, "protocol", transport) &&
	  !OU::findString(otherParams, "transport", transport) &&
	  !OU::findString(otherParams, "endpoint", transport))
	transport = preferred;
      if (transport) {
	unsigned n = 0;
	newParams = new OU::PValue[toParams->length() + 2];
	for (const OU::PValue *p = toParams; p && p->name; p++, n++)
	  newParams[n] = *p;
	newParams[n].name = "transport";
	newParams[n].vString = transport;
	newParams[n].type = OA::OCPI_String;
	newParams[++n].name = NULL;
	toParams = newParams;
      }
    }

    // The general case of connecting ports that are managed in the same process.
    void Port::connect(OA::Port &apiOther, const OU::PValue *myParams,
		       const OU::PValue *otherParams) {
      OU::SelfAutoMutex guard (this);
      Port &other = *static_cast<Port*>(&apiOther);
      //      setMode( CON_TYPE_RDMA );
      //      other.setMode( CON_TYPE_RDMA );
      if (isProvider())
        if (other.isProvider())
          throw OU::Error("Cannot connect two provider ports");
        else
          other.connect( *this, otherParams, myParams);
      else if (!other.isProvider())
        throw OU::Error("Cannot connect to user ports");
      else {
        Container &otherContainer = other.container();
	// FIXME: Take any connection-related parameters and make sure both parameter lists have them.
        // Containers know how to do internal connections
        if (&m_container == &otherContainer) {
	  other.setConnectParams(otherParams);
          connectInside(other, myParams, otherParams);
          // Container MAY know how to do intercontainer connections between like containers.
	  //        } else if (&container().driver() == &otherContainer.driver() &&
	  //		   connectLike( other, myParams, otherParams))
	  //	  return;
	} else {
	  const char *preferred = getPreferredProtocol();
	  // Check if the output side has a preferred protocol, and if so, set it
	  if (!preferred)
	    preferred = other.getPreferredProtocol();
	  // Ensure that any connect parameters are on both ports' lists
	  OU::PValue *newMyParams, *newOtherParams;
	  mergeConnectParams(otherParams, preferred, myParams, newMyParams);
	  mergeConnectParams(myParams, preferred, otherParams, newOtherParams);
#if 1
	  if (!other.m_canBeExternal)
	    throw OU::Error("Port \"%s\" of \"%s\" cannot be connected external to container",
			    other.m_metaPort.m_name.c_str(), worker().name().c_str());
	  if (!m_canBeExternal)
	    throw OU::Error("Port \"%s\" of \"%s\" cannot be connected external to container",
			    m_metaPort.m_name.c_str(), worker().name().c_str());
	  other.setConnectParams(otherParams);
	  setConnectParams(myParams);
	  determineRoles(other.getData().data);
	  other.startConnect(NULL, otherParams);
	  startConnect(&other.getData().data, myParams);
	  const Descriptors *outDesc;
	  Descriptors feedback;
	  // try to finish output side, possibly producing flow control feedback
	  if ((outDesc = finishConnect(other.getData().data, feedback)))
	    // try to finish input side, possibly providing some feedback
	    // see setInitialUserInfo below
	    if ((outDesc = other.finishConnect(*outDesc, feedback)))
	      // in fact more is needed to finish on output side
	      // - like enabling it to start sending since the receiver is now ready
	      // see setFinalProviderInfo below
	      if ((outDesc = finishConnect(*outDesc, feedback)))
		// see setFinalUserInfo
		other.finishConnect(*outDesc, feedback);
#else
	  std::string pInfo, uInfo;
	  other.getInitialProviderInfo(otherParams, pInfo);
	  // FIXME: make this a proper automatic to avoid exception leaks
	  if (newOtherParams)
	      delete [] newOtherParams;
	  setInitialProviderInfo(myParams, pInfo, uInfo);
	  if (newMyParams)
	      delete [] newMyParams;
          if (!uInfo.empty()) {
            other.setInitialUserInfo(uInfo, pInfo);
            if (!pInfo.empty()) {
	      setFinalProviderInfo(pInfo, uInfo);
              if (!uInfo.empty())
                other.setFinalUserInfo(uInfo);
            }
	  }
#endif
        }
      }
    }

    void Port::connectURL(const char*, const OU::PValue *,
			  const OU::PValue *) {
      ocpiDebug("connectURL not allowed on this container !!");
      ocpiAssert( 0 );
    }

    // Start the remote/intercontainer connection process
    void Port::getInitialProviderInfo(const OU::PValue *params, std::string &out) {
      OU::SelfAutoMutex guard (this);
      ocpiAssert(isProvider());
      if (!m_canBeExternal)
	throw OU::Error("Port \"%s\" cannot be connected external to container",
			m_metaPort.m_name.c_str());
      applyConnectParams(NULL, params);
      packPortDesc(getData().data, out);
    }

    // User/output side initial method, that carries provider info and returns user info
    bool Port::setInitialProviderInfo(const OU::PValue *params,
				       const std::string &ipi, std::string &out) {
      OU::SelfAutoMutex guard (this);
      // User side, producer side.
      ocpiAssert(!isProvider());
      if (!m_canBeExternal)
	throw OU::Error("Port \"%s\" cannot be connected external to container",
			m_metaPort.m_name.c_str());
      Descriptors otherPortData;
      unpackPortDesc(ipi, otherPortData);
      // Adjust any parameters from connection metadata
      applyConnectParams(&otherPortData, params);
      // We now know the role aspects of both sides.  Make the decision so we know what
      // resource allocations to make in finishConnect.
      determineRoles(otherPortData);
      Descriptors feedback;
      const Descriptors *outDesc;
      // This "finish" might be provisional - i.e.we might get more info
      if ((outDesc = finishConnect(otherPortData, feedback))) {
	packPortDesc(*outDesc, out);
	return true;
      } else {
	out.clear();
	return false;
      }
    }

    // Input side being told about output side
    bool Port::setInitialUserInfo(const std::string &iui, std::string &out) {
      OU::SelfAutoMutex guard (this);
      ocpiAssert(isProvider());
      Descriptors otherPortData;
      unpackPortDesc(iui, otherPortData);
      // Conceivably we would determine roles here.
      determineRoles(otherPortData);
      Descriptors feedback;
      const Descriptors *outDesc;
      if ((outDesc = finishConnect(otherPortData, feedback))) {
	packPortDesc(*outDesc, out);
	return true;
      } else {
	out.clear();
	return false;
      }
    }

    // User only
    bool Port::setFinalProviderInfo(const std::string &fpi, std::string &out) {
      OU::SelfAutoMutex guard (this);
      ocpiAssert(!isProvider());
      Descriptors otherPortData;
      unpackPortDesc(fpi, otherPortData);
      Descriptors feedback;
      const Descriptors *outDesc;
      if ((outDesc = finishConnect(otherPortData, feedback))) {
	packPortDesc(*outDesc, out);
	return true;
      } else {
	out.clear();
	return false;
      }
    }
    // Provider Only
    void Port::setFinalUserInfo(const std::string &fui) {
      OU::SelfAutoMutex guard (this);
      ocpiAssert(!isProvider());
      Descriptors otherPortData;
      unpackPortDesc(fui, otherPortData);
      Descriptors feedback;
      if (finishConnect(otherPortData, feedback))
	throw OU::Error("Unexpected output from setFinalUserInfo");
    }
    // Establish the roles, which might happen earlier than the finalization of the connection
    // Since roles can determine resource allocations
    // This could be table-driven...
    void Port::determineRoles(Descriptors &other) {
      static const char *roleName[] =
	{"ActiveMessage", "ActiveFlowControl", "ActiveOnly", "Passive", "MaxRole", "NoRole"};

      Descriptors
        &pDesc = isProvider() ? getData().data : other,
        &uDesc = isProvider() ? other : getData().data;
      ocpiInfo("Port %s of %s, a %s, has options 0x%x, initial role %s, buffers %u size %u",
	       m_metaPort.m_name.c_str(), worker().name().c_str(),
	       isProvider() ? "provider/consumer" : "user/producer",
		getData().data.options, roleName[getData().data.role],
		getData().data.desc.nBuffers, getData().data.desc.dataBufferSize);
      ocpiInfo("  other has options 0x%x, initial role %s, buffers %u size %u",
		other.options, roleName[other.role], other.desc.nBuffers, other.desc.dataBufferSize);
      chooseRoles(uDesc.role, uDesc.options, pDesc.role, pDesc.options);
      ocpiInfo("  after negotiation, port %s, a %s, has role %s,"
	       "  other has role %s",
	       m_metaPort.m_name.c_str(), isProvider() ? "provider/consumer" : "user/producer",
		roleName[getData().data.role], roleName[other.role]);
      size_t maxSize =  pDesc.desc.dataBufferSize;
      if (uDesc.desc.dataBufferSize > pDesc.desc.dataBufferSize)
	maxSize = uDesc.desc.dataBufferSize;
      maxSize = OU::roundUp(maxSize, BUFFER_ALIGNMENT);
      if (maxSize > pDesc.desc.dataBufferSize) {
	// Expanding the input size buffers
	pDesc.desc.dataBufferSize = OCPI_UTRUNCATE(uint32_t, maxSize);
      }
      if (maxSize > uDesc.desc.dataBufferSize) {
	// Expanding the output size buffer
	uDesc.desc.dataBufferSize = OCPI_UTRUNCATE(uint32_t, maxSize);
      }
      // FIXME: update bufferSizePort relationships to ports that are changing their size
      // But this depends on the order of the connections..
      // but this should only happen with runtime-parameter buffer sizes
      // perhaps make it an error
      ocpiInfo("  after negotiation, buffer size is %zu", maxSize);
      // We must make sure other side doesn't mess with roles anymore.
      uDesc.options |= 1 << MandatedRole;
      pDesc.options |= 1 << MandatedRole;
    }


    /*
     * ----------------------------------------------------------------------
     * A simple test.
     * ----------------------------------------------------------------------
     */
    /*
      static int
      pack_unpack_test (int argc, char *argv[])
      {
      Descriptors d;
      std::string data;
      bool good;

      std::memset (&d, 0, sizeof (Descriptors));
      d.mode = ConsumerDescType;
      d.desc.c.fullFlagValue = 42;
      std::strcpy (d.desc.c.oob.oep, "Hello World");
      data = packDescriptor (d);
      std::memset (&d, 0, sizeof (Descriptors));
      good = unpackDescriptor (data, d);
      ocpiAssert (good);
      ocpiAssert (d.mode == ConsumerDescType);
      ocpiAssert (d.desc.c.fullFlagValue == 42);
      ocpiAssert (std::strcmp (d.desc.c.oob.oep, "Hello World") == 0);

      std::memset (&d, 0, sizeof (Descriptors));
      d.mode = ProducerDescType;
      d.desc.p.emptyFlagValue = 42;
      std::strcpy (d.desc.p.oob.oep, "Hello World");
      data = packDescriptor (d);
      std::memset (&d, 0, sizeof (Descriptors));
      good = unpackDescriptor (data, d);
      ocpiAssert (good);
      ocpiAssert (d.mode == ProducerDescType);
      ocpiAssert (d.desc.p.emptyFlagValue == 42);
      ocpiAssert (std::strcmp (d.desc.p.oob.oep, "Hello World") == 0);

      data[0] = ((data[0] == '\0') ? '\1' : '\0'); // Hack: flip byteorder
      good = unpackDescriptor (data, d);
      ocpiAssert (!good);

      return 0;
      }
    */
    static void putOffset(OU::CDR::Encoder &packer, DtOsDataTypes::Offset val) {
      packer.
#if OCPI_EP_SIZE_BITS == 64
      putULongLong(val);
#else
      putULong(val);
#endif
    }
    static void putFlag(OU::CDR::Encoder &packer, DtOsDataTypes::Flag val) {
      packer.
#if OCPI_EP_FLAG_BITS == 64
      putULongLong(val);
#else
      putULong(val);
#endif
    }
    static void getOffset(OU::CDR::Decoder &unpacker, DtOsDataTypes::Offset &val) {
      unpacker.
#if OCPI_EP_SIZE_BITS == 64
      getULongLong(val);
#else
      getULong(val);
#endif
    }
    static void getFlag(OU::CDR::Decoder &unpacker, DtOsDataTypes::Flag &val) {
      unpacker.
#if OCPI_EP_FLAG_BITS == 64
      getULongLong(val);
#else
      getULong(val);
#endif
    }

    void Port::packPortDesc(const Descriptors & desc, std::string &out)
      throw()
    {
      OU::CDR::Encoder packer;
      packer.putBoolean (OU::CDR::nativeByteorder());
      packer.putULong     (desc.type);
      packer.putULong     (desc.role);
      packer.putULong     (desc.options);
      const Desc_t & d = desc.desc;
      packer.putULong     (d.nBuffers);
      putOffset(packer, d.dataBufferBaseAddr);
      packer.putULong     (d.dataBufferPitch);
      packer.putULong     (d.dataBufferSize);
      putOffset(packer, d.metaDataBaseAddr);
      packer.putULong     (d.metaDataPitch);
      putOffset(packer, d.fullFlagBaseAddr);
      packer.putULong     (d.fullFlagSize);
      packer.putULong     (d.fullFlagPitch);
      putFlag(packer, d.fullFlagValue);
      putOffset(packer, d.emptyFlagBaseAddr);
      packer.putULong     (d.emptyFlagSize);
      packer.putULong     (d.emptyFlagPitch);
      putFlag(packer, d.emptyFlagValue);
      packer.putULongLong (d.oob.port_id);
      packer.putString    (d.oob.oep);
      packer.putULongLong (d.oob.cookie);
      out = packer.data();
    }

    bool Port::unpackPortDesc(const std::string &data, Descriptors &desc)
      throw ()
    {
      OU::CDR::Decoder unpacker (data);

      try { 
	bool bo;
	unpacker.getBoolean (bo);
	unpacker.byteorder (bo);
        unpacker.getULong (desc.type);
        unpacker.getLong (desc.role);
        unpacker.getULong (desc.options);
	Desc_t & d = desc.desc;
	unpacker.getULong     (d.nBuffers);
	getOffset(unpacker, d.dataBufferBaseAddr);
	unpacker.getULong     (d.dataBufferPitch);
	unpacker.getULong     (d.dataBufferSize);
	getOffset(unpacker, d.metaDataBaseAddr);
	unpacker.getULong     (d.metaDataPitch);
	getOffset(unpacker, d.fullFlagBaseAddr);
	unpacker.getULong     (d.fullFlagSize);
	unpacker.getULong     (d.fullFlagPitch);
	getFlag(unpacker, d.fullFlagValue);
	getOffset(unpacker, d.emptyFlagBaseAddr);
	unpacker.getULong     (d.emptyFlagSize);
	unpacker.getULong     (d.emptyFlagPitch);
	getFlag(unpacker, d.emptyFlagValue);
	unpacker.getULongLong (d.oob.port_id);
        std::string oep;
	unpacker.getString (oep);
        if (oep.length()+1 > sizeof(d.oob.oep))
          return false;
	unpacker.getULongLong (d.oob.cookie);
        std::strcpy (d.oob.oep, oep.c_str());
      }
      catch (const OU::CDR::Decoder::InvalidData &) {
	return false;
      }
      return true;
    }

    namespace {
      void defaultRole(int32_t &role, uint32_t options) {
	if (role == NoRole) {
	  for (unsigned n = 0; n < MaxRole; n++)
	    if (options & (1 << n)) {
	      role = n;
	      return;
	    }
	  throw OU::Error("Container port has no transfer roles");
	}
      }
    }

    // coming in, specified roles are preferences or explicit instructions.
    // The existing settings are either NoRole, a preference, or a mandate
    void BasicPort::chooseRoles(int32_t &uRole, uint32_t uOptions, int32_t &pRole, uint32_t pOptions) {
      // FIXME this relies on knowledge of the values of the enum constants
      static PortRole otherRoles[] =
        {ActiveFlowControl, ActiveMessage,
         Passive, ActiveOnly};
      defaultRole(uRole, uOptions);
      defaultRole(pRole, pOptions);
      PortRole
        pOther = otherRoles[pRole],
        uOther = otherRoles[uRole];
      if (pOptions & (1 << MandatedRole)) {
        // provider has a mandate
        ocpiAssert(pRole != NoRole);
        if (uRole == pOther)
          return;
        if (uOptions & (1 << MandatedRole))
          throw OU::Error("Incompatible mandated transfer roles");
        if (uOptions & (1 << pOther)) {
          uRole = pOther;
          return;
        }
        throw OU::Error("No compatible role available against mandated role");
      } else if (pRole != NoRole) {
        // provider has a preference
        if (uOptions & (1 << MandatedRole)) {
          // user has a mandate
          ocpiAssert(uRole != NoRole);
          if (pRole == uOther)
            return;
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          throw OU::Error("No compatible role available against mandated role");
        } else if (uRole != NoRole) {
          // We have preferences on both sides, but no mandate
          // If preferences match, all is well
          if (pRole == uOther)
            return;
          // If one preference is against push, we better listen to it.
          if (uRole == ActiveFlowControl &&
              pOptions & (1 << ActiveMessage)) {
            pRole = ActiveMessage;
            return;
          }
          // Let's try active push if we can
          if (uRole == ActiveMessage &&
              pOptions & (1 << ActiveFlowControl)) {
            pRole = ActiveFlowControl;
            return;
          }
          if (pRole == ActiveFlowControl &&
              uOptions & (1 << ActiveMessage)) {
            uRole = ActiveFlowControl;
            return;
          }
          // Let's try activeonly push if we can
          if (uRole == ActiveOnly &&
              pOptions & (1 << Passive)) {
            pRole = Passive;
            return;
          }
          if (pRole == Passive &&
              pOptions & (1 << ActiveOnly)) {
            pRole = ActiveOnly;
            return;
          }
          // Let's give priority to the "better" role.
          if (uRole < pRole &&
              pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          // Give priority to the provider
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return;
          }
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          // Can't use either preference.  Fall throught to no mandates, no preferences
        } else {
          // User role unspecified, but provider has a preference
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return;
          }
          // Can't use provider preference, Fall through to no mandates, no preferences
        }
      } else if (uOptions & (1 << MandatedRole)) {
        // Provider has no mandate or preference, but user has a mandate
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return;
        }
        throw OU::Error("No compatible role available against mandated role");
      } else if (uRole != NoRole) {
        // Provider has no mandate or preference, but user has a preference
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return;
        }
        // Fall through to no mandates, no preferences.
      }
      // Neither has useful mandates or preferences.  Find anything, biasing to push
      for (int i = 0; i < MaxRole; i++)
        // Provider has no mandate or preference
        if (uOptions & (1 << i) &&
            pOptions & (1 << otherRoles[i])) {
          uRole = i;
          pRole = otherRoles[i];
          return;
        }
      throw OU::Error("No compatible combination of roles exist");
    }            

    ExternalBuffer::
    ExternalBuffer() :
      m_dtBuffer(NULL), m_dtPort(NULL)
    {}
    void ExternalBuffer::
    release() {
      if (m_dtBuffer) {
	m_dtPort->releaseInputBuffer(m_dtBuffer);
	m_dtBuffer = NULL;
      }
    }
    void ExternalBuffer::
    put( size_t length, uint8_t opCode, bool /*endOfData*/) {
      m_dtPort->sendOutputBuffer(m_dtBuffer, length, opCode);
      m_dtBuffer = NULL;
    }
    // Producer or consumer
    ExternalPort::
    ExternalPort(Port &port, bool isProvider, const OU::PValue *extParams, 
		 const OU::PValue *portParams)
      // FIXME: push the xfer options down to the lower layers
      : BasicPort(port.metaPort(), isProvider,
		  (1 << OCPI::RDT::ActiveFlowControl) |
		  (1 << OCPI::RDT::ActiveMessage), port, extParams),
	m_dtPort(NULL)
    {
      const char *preferred = port.getPreferredProtocol();
      OU::PValue *newExtParams, *newPortParams;
      // Grab any connection parameters from both param lists to both lists.
      // I.e. ensure that any connection parameters are on both lists.
      mergeConnectParams(portParams, preferred, extParams, newExtParams);
      mergeConnectParams(extParams, preferred, portParams, newPortParams);
      if (isProvider) {
	// Create the DT input port which is the basis for this external port
	// Apply our params to the basic port
	applyConnectParams(NULL, extParams);
	// Create the DT port using our merged params
	m_dtPort = port.container().getTransport().createInputPort(getData().data, extParams);
	// Start the connection process on the worker port
	if (port.isLocal()) {
	  port.applyConnectParams(NULL, portParams); 
	  port.determineRoles(getData().data);
	  port.localConnect(*m_dtPort);
	} else {
	  port.applyConnectParams(&getData().data, portParams);
	  port.determineRoles(getData().data);
	}
	// Finalize the worker's output port, getting back the flow control descriptor
	Descriptors feedback;
	const Descriptors *outDesc = port.finishConnect(getData().data, feedback);
	if (outDesc)
	  m_dtPort->finalize(*outDesc, getData().data);
      } else {
	port.applyConnectParams(NULL, portParams);
	if (port.isLocal())
	  m_dtPort = port.container().getTransport().
	    createOutputPort(getData().data, port.dtPort());
	else
	  m_dtPort = port.container().getTransport().
	    createOutputPort(getData().data, port.getData().data);
	port.determineRoles(getData().data);
	Descriptors localShadowPort, feedback;
	const Descriptors *outDesc = m_dtPort->finalize(port.getData().data, getData().data, &localShadowPort);
	ocpiCheck(!port.finishConnect(*outDesc, feedback));
      }
      m_lastBuffer.m_dtPort = m_dtPort;
      delete [] newExtParams;
      delete [] newPortParams;
    }
    ExternalPort::
    ~ExternalPort() {
      if (m_dtPort)
	m_dtPort->reset();
    }
    OA::ExternalBuffer *ExternalPort::
    getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &end) {
      if (!isProvider())
	throw OU::Error("getBuffer on input port called on external output port %s",
			name().c_str());
      if (m_lastBuffer.m_dtBuffer != NULL)
	throw OU::Error("getBuffer called on input port %s without releasing previous buffer",
			name().c_str());
			
      end = false;
      void *vdata;
      if ((m_lastBuffer.m_dtBuffer = m_dtPort->getNextFullInputBuffer(vdata, length, opCode))) {
	data = (uint8_t*)vdata; // fix all the buffer data types to match the API: uint8_t*
	return &m_lastBuffer;
      }
      return NULL;
    }
    OA::ExternalBuffer *ExternalPort::
    getBuffer(uint8_t *&data, size_t &length) {
      if (isProvider())
	throw OU::Error("getBuffer for output port called on input port %s",
			name().c_str());
      if (m_lastBuffer.m_dtBuffer != NULL)
	throw OU::Error("getBuffer called on output port %s without sending previous buffer",
			name().c_str());
      void *vdata;
      if ((m_lastBuffer.m_dtBuffer = m_dtPort->getNextEmptyOutputBuffer(vdata, length))) {
	data = (uint8_t*)vdata; // fix all the buffer data types to match the API: uint8_t*
	return &m_lastBuffer;
      }
      return NULL;
    }
    void ExternalPort::
    endOfData() {
      ocpiAssert("No EndOfData support for external ports"==0);
    }
    bool ExternalPort::
    tryFlush() {
      return false;
    }
    OA::ExternalPort &Port::
    connectExternal(const char *extName, const OA::PValue *portParams,
		    const OA::PValue *extParams) {
      if (!m_canBeExternal)
        throw OU::ApiError ("For external port \"", extName, "\", port \"",
			    name().c_str(), "\" of worker \"",
			    worker().implTag().c_str(), "/", worker().instTag().c_str(), "/",
			    worker().name().c_str(),
			    "\" is locally connected in the HDL bitstream. ", NULL);
      // This call should get the worker's port ready to be connected.
      return createExternal(extName, !isProvider(), extParams, portParams);
    }
  }

  namespace API {
    ExternalBuffer::~ExternalBuffer(){}
    ExternalPort::~ExternalPort(){}
    Port::~Port(){}
  }
}
