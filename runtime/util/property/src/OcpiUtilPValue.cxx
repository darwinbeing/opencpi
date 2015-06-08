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
#include <memory>
#include <unistd.h>
#include <stdarg.h>
#include <strings.h>
#include <assert.h>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilDataTypes.h"
#include "OcpiUtilValue.h"
#include "OcpiPValue.h"

namespace OU = OCPI::Util;
namespace OE = OCPI::Util::EzXml;
namespace OA = OCPI::API;
namespace OCPI {
  namespace API {
    PVULong PVEnd(0,0);

    unsigned PValue::length() const {
      unsigned n = 0;
      if (this) // FIXME - this is really not kosher.
	for (const PValue *p = this; p->name; p++, n++)
	  ;
      return n;
    }
    const std::string &PValue::unparse(std::string &sval, bool add) const {
      OU::ValueType vtype(type);
      OU::Value val(vtype);
      // FIXME: PValues and Values must be better integrated...
      switch (type) {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
	case OA::OCPI_##pretty: val.m_##pretty = v##pretty; break;
	OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      default:;
      }
      val.unparse(sval, NULL, add);
      return sval;
    }
  }
  namespace Util {
    // This list provides names and types and defaults
    PValue allPVParams[] = {
      PVString("transport"),
      PVString("xferrole"),
      PVString("DLLEntryPoint"),
      PVString("monitorIPAddress"),
      PVString("protocol"),
      PVString("endpoint"),
      PVString("Device"),
      PVBool("ownthread"),
      PVBool("polled"),
      PVULong("bufferCount"),
      PVULong("bufferSize"),
      PVUChar("index"),
      PVString("interconnect"),
      PVString("adapter"),
      PVString("configure"),
      PVString("io"),
      PVString("paramconfig"),
      PVString("model"),
      PVString("platform"),
      PVBool("signal"),
      PVEnd
    };

    PVULong PVEnd(0,0);
    static const PValue *
    find(const PValue* p, const char* name) {
      if (p)
	for (; p->name; p++)
	  if (!strcasecmp(p->name, name))
	    return p;
      return NULL;
    }
 
#define OCPI_DATA_TYPE(sca, corba, letter, bits, run, pretty, store)\
    bool 							    \
    find##pretty(const PValue* p, const char* name, run &value) {   \
      const PValue *fp = find(p, name);				    \
      if (fp) {							    \
        if (fp->type == OA::OCPI_##pretty) {	                    \
          value = fp->v##pretty;				    \
          return true;						    \
	} else							    \
	  throw Error("Property \"%s\" is not a %s", name, #pretty);\
      }                                                             \
      return false;						    \
    }                                                               \
    void 							    \
    add##pretty(const PValue*&p, const char* name, run value) {     \
      PValue pv[] = { PV##pretty(name, value), PVEnd };             \
      p = (new PValueList(p, pv))->list();			    \
    }

    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
    //#undef OCPI_DATA_TYPE_S
    //#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

    bool 
    findAssign(const PValue *p, const char *name, const char *var, std::string &sval) {
      const char *val;
      if (findAssign(p, name, var, val)) {
	sval = val;
	return true;
      }
      return false;
    }
    bool 
    findAssign(const PValue *p, const char *name, const char *var, const char *&val) {
      if (p)
	for (; p->name; p++)
	  if (!strcasecmp(p->name, name)) {
	    if (p->type == OA::OCPI_String) {
	      size_t len = p->vString[0] == '=' ? 0 : strlen(var);
	      if (len == 0 ||
		  !strncasecmp(var, p->vString, len) && p->vString[len] == '=') {
		val = p->vString + len + 1;
		return true;
	      }
	    } else
	      throw ApiError("Parameter \"", name, "\" is not a string", NULL);
	  }
      return false;
    }

    bool 
    findAssignNext(const PValue *p, const char *name, const char *var,
		   const char *&val, unsigned &next) {
      if (p)
	for (unsigned n = 0; p->name; p++, n++)
	  if (n >= next && !strcasecmp(p->name, name)) {
	    if (p->type == OA::OCPI_String) {
	      if (!var) {
		val = p->vString;
		next = n + 1;
		return true;
	      } else {
		size_t len = p->vString[0] == '=' ? 0 : strlen(var);
		if (len == 0 ||
		    !strncasecmp(var, p->vString, len) && p->vString[len] == '=') {
		  val = p->vString + len + 1;
		  next = n + 1;
		  return true;
		}
	      }
	    } else
	      throw ApiError("Parameter \"", name, "\" is not a string", NULL);
	  }
      return false;
    }

    PValueList::PValueList() : m_list(NULL) {}
    PValueList::PValueList(const PValue *params, const PValue *override) : m_list(NULL) {
      add(params, override);
    }
    PValueList & PValueList::
    operator=(const PValueList & p ) {
      delete [] m_list;
      m_list = NULL;
      add(p.m_list);
      return *this;
    }
    void PValueList::
    add(const PValue *params, const PValue *override) {
      size_t n = m_list ? m_list->length() : 0;
      n += params ? params->length() : 0;
      n += override ? override->length() : 0;
      if (!n)
	return;
      PValue *old = m_list;
      PValue *p = m_list = new PValue[n + 1];
      for (const PValue *op = override; op && op->name; op++)
	*p++ = *op;
      for (const PValue *pp = params; pp && pp->name; pp++) {
	for (const PValue *xp = m_list; xp < p; xp++)
	  if (!strcasecmp(pp->name, xp->name))
	    goto skipit1;
	*p++ = *pp;
      skipit1:;
      }
      for (const PValue *pp = old; pp && pp->name; pp++) {
	for (const PValue *xp = m_list; xp < p; xp++)
	  if (!strcasecmp(pp->name, xp->name))
	    goto skipit2;
	*p++ = *pp;
      skipit2:;
      }
      *p = PVEnd;
    }

    static const char *parseParam(const char *name, const char *value, PValue &p) {
      const PValue *allP = find(allPVParams, name);
      if (!allP)
	return esprintf("parameter named \"%s\" not defined, misspelled?", name);
      p.name = allP->name;
      p.type = allP->type;
      ValueType type(allP->type);
      Value val(type);
      const char *err = val.parse(value);
      if (err)
	return err;
      if (p.type == OA::OCPI_String) {
	p.vString = strdup(val.m_String);
	p.owned = true;
      } else
	p.vULongLong = val.m_ULongLong;
      return NULL;
    }

    PValueList::~PValueList() { delete [] m_list; }
    const char *PValueList::parse(ezxml_t x, ...) {
      va_list ap;
      va_start(ap, x);
      const char *err = vParse(NULL, x, ap);
      va_end(ap);
      return err;
    }
    const char *PValueList::parse(const PValue * pvl, ezxml_t x, ...) {
      va_list ap;
      va_start(ap, x);
      const char *err = vParse(pvl, x, ap);
      va_end(ap);
      return err;
    }
    const char *PValueList::vParse(const PValue *pvl, ezxml_t x, va_list ap) {
      unsigned nPvl = pvl ? pvl->length() : 0;
      unsigned nXml = OE::countAttributes(x);
      unsigned n = nPvl + nXml;
      if (!n) {
	m_list = NULL;
	return NULL;
      }
      PValue *p = m_list = new PValue[n + 1];
      for (unsigned n = 0; n < nPvl; n++)
	*p++ = pvl[n];
      const char *name, *value;
      EZXML_FOR_ALL_ATTRIBUTES(x, name, value) {
	const char *attr;
	bool found = false;
	va_list dest;
	va_copy(dest, ap);
	while ((attr = va_arg(dest, const char *)))
	  if (!strcasecmp(name, attr)) {
	    found = true;
	    break;
	  }
	va_end(dest);
	if (!found) {
	  const char *err = parseParam(name, value, *p);
	  if (err)
	    return err;
	  p++;
	}
      }
      p->name = NULL;
      return NULL;
    }
    const char *PValueList::addXml(ezxml_t x) {
      unsigned nPvl = m_list ? m_list->length() : 0;
      unsigned nXml = OE::countAttributes(x);
      unsigned n = nPvl + nXml;
      if (!n) {
	m_list = NULL;
	return NULL;
      }
      PValue *old = m_list;
      PValue *p = m_list = new PValue[n + 1];
      while (old && old->name)
	*p++ = *old++;
      const char *name, *value;
      EZXML_FOR_ALL_ATTRIBUTES(x, name, value) {
	const char *err = parseParam(name, value, *p);
	if (err)
	  return err;
	p++;
      }
      p->name = NULL;
      return NULL;
    }
    const char *PValueList::
    add(const char *name, const char *value) {
      PValue newp;
      const char *err;
      if ((err = parseParam(name, value, newp)))
	return err;
      PValue *oldp = m_list;
      PValue *p = m_list = new PValue[(m_list ? m_list->length() : 0) + 2];
      for (const PValue *op = oldp; op && op->name; op++)
	*p++ = *op;
      *p++ = newp;
      *p++ = PVEnd;
      delete [] oldp;
      return NULL;
    }
  }
}