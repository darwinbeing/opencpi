
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



/*
 * Abstact:
 *   This file contains the class that controls the setup for handshake within the OCPI transfer protocol.
 *
 * Revision History:
 *
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef DataTransfer_HandShakeControl_H_
#define DataTransfer_HandShakeControl_H_

#include <DtSharedMemoryInterface.h>
#include <DtOsDataTypes.h>

namespace DataTransfer {

  // This is the const value that is used to determine if a SMB transport
  // is up and running
  typedef uint64_t UpAndRunning;
  const UpAndRunning UpAndRunningMarker = 0x51abac;

  // Token definition
  typedef int32_t ControlToken;
  typedef int32_t BooleanToken;
  typedef int32_t PortId; // signed to allow a sentinel of -1

  /**********************************
   *  SMB communications structures
   *
   *         This structure is used to make inband requests to other SMB's.
   *    We use our mailbox id to index into our local SMB as the output
   *  of our request.  A non-zero in the request indicates that a request
   *  is in process.  If this local SMB is used in a multi-threaded environment
   *  this structure needs mutex protection.
   *********************************/
  const int GetPortOffsets = 1;

  struct ContainerComms {

    enum ReqTypeIds {
      NoRequest = 0,
      ReqShadowRstateOffset=1,
      ReqInputOffsets=2,
      ReqOutputControlOffset=3,
      ReqNewConnection = 4,
      ReqUpdateCircuit = 5
    };

    struct RequestHeader {
      ReqTypeIds type;
      uint32_t   pad;
      uint32_t   circuitId; // non-zero when needed
    };
    struct RequestUpdateCircuit {
      uint32_t   senderCircuitId;           // Id of circuit of interest
      PortId     receiverPortId;            // Our new input port id
      uint32_t   tPortCount;                // Number of input ports
      PortId     senderPortId;              // Sender port ordinal
      PortId     senderOutputPortId;        // Our output port ordinal
      uint64_t   senderOutputControlOffset; // Control offset to output
      uint32_t   pad1;
      char       output_end_point[128];     // Output endpoint
    };

    struct RequestShadowRstateOffset {
      uint32_t   portId;
      uint32_t   pad1;
      char       url[128];
    };

    struct RequestInputOffsets {
      uint32_t   portId;
      uint32_t   pad1;
      char       url[128];
    };

    struct RequestOutputControlOffset {
      uint32_t   portId;
      DtOsDataTypes::Offset protocol_offset; // server side telling us where to put the protocol info
      char       shadow_end_point[128];      // Output endpoint
    };

    struct RequestNewConnection {
      uint32_t   buffer_size;
      uint32_t   send;                   // Send or recieve buffer boolean
      uint32_t   protocol_size;          // the size of the client's protocol info
      uint32_t   pad1;
      char       output_end_point[128];
    };

    struct Request {
      RequestHeader header;
      union {
	RequestInputOffsets         reqInputOffsets;
	RequestShadowRstateOffset   reqShadowOffsets;
	RequestOutputControlOffset  reqOutputContOffset;
	RequestNewConnection        reqNewConnection;
	RequestUpdateCircuit        reqUpdateCircuit;
      };
    };

    struct MailBox {
      Request               request;
      int32_t               error_code;
      uint32_t              returnMailboxId; // FIXME: make this the right type
      DtOsDataTypes::Offset return_offset;   // unused when size is zero
      uint32_t              return_size;
    };

    UpAndRunning             upAndRunning;
    MailBox                  mailBox[DtOsDataTypes::MAX_SYSTEM_SMBS+1];// +1 is required, prevents crashes, but no root cause yet.

  };

  // This class is used to manage the endpoints mailbox
  class XferMailBox {

  public:

    // Constructor 
    XferMailBox(uint16_t slot )
      :m_slot(slot){};

      // This method sets the communications slot for this template
      void setMailBox(uint16_t slot ){m_slot=slot;};

      // Determine if the mail box is avialable
      inline bool mailBoxAvailable( SMBResources* res ){ 
        return (res->m_comms->mailBox[m_slot].request.header.type == ContainerComms::NoRequest) ? true : false;
      }

      // Returns the pointer to the mailbox
      inline ContainerComms::MailBox* getMailBox(SMBResources* res){return &res->m_comms->mailBox[m_slot];}

      // This method makes a mailbox request from our local dedicated mailbox slot to 
      // our remote dedicated mailbox slot.
      bool makeRequest(SMBResources* output, SMBResources* input );

  protected:

      // Our mail slot index
      uint16_t m_slot;

  };


  /**********************************
   *  Output port control structure
   *********************************/
  struct OutputPortSetControl {

    // This token is used to control what output port can transfer
    ControlToken    sequentialControlToken;

    BooleanToken   endOfWhole;           // end of whole data distribution
    BooleanToken   endOfStream;          // end of data stream

    uint32_t           numberOfBuffers;
  };


  union BufferState {
    // buffer full token
    uint32_t   bufferIsFull, bufferIsEmpty ;
  };


  struct BufferShape
  {
    /* Return error value */
    uint32_t error;

    /* Buffer handle */
    uint32_t buffer;

    /* Number of dimensions */
    uint32_t ndims;

    /* Data shape size */
    uint32_t dataShape[3];

    /* Whole shape size */
    uint32_t wholeShape[3];

    /* Left overlap sizes */
    uint32_t left[3];

    /* Right overlap sizes */
    uint32_t right[3];

    /* Position of data in whole */
    uint32_t wholePosition[3];

    /* Strides of data in buffer */
    uint32_t dataInBuffer[3];

    /* Offset from start of buffer to start of data */
    uint32_t dataBufferOffset;
  };



  /**********************************
   *  Buffer meta-data
   *********************************/
  const unsigned int ZeroCopyReady = 0x10000000;

  const unsigned
    lengthBits = 22, // 8Mbytes - 1
    opCodeBits = 8;

  // This packing is simply to make it easier to read the values in hex dumps
  const uint32_t maxXferLength = (1 << 22) - 1;
  inline uint32_t packXferMetaData(size_t length, uint8_t opcode, bool eof) {
    assert(length <= maxXferLength);
    return (uint32_t)
      ((opcode << (32 - opCodeBits)) | // high byte
       (1 << (lengthBits+1)) |         // 800000 always set to ensure it is non-zero
       ((eof ? 1 : 0) << lengthBits) | // 400000 EOF independent of length
       length);                         // length in bytes up to 2^22-1
  }
  inline void unpackXferMetaData(uint32_t md, size_t &length, uint8_t &opcode, bool &eof) {
    assert(md & (1 << (lengthBits+1)));
    length = md & ~(-1 << lengthBits);
    opcode = (uint8_t)((md >> (32 - opCodeBits)) & ((1 << opCodeBits) - 1));
    eof = md & (1 << lengthBits) ? true : false;
  }
  struct RplMetaData {
    uint32_t length;
    uint8_t opCode;
    uint8_t end;
    uint32_t xferMetaData; // the compressed version when required.
    uint32_t timestamp;
  };


  struct BufferMetaData {
    RplMetaData              ocpiMetaDataWord;      // OCPI compatible metadata word

    uint32_t           sequence;                            // Transfer sequence
    int32_t           userTag;                            // User defined buffer tag
    int32_t           endOfCircuit;                    // Circuit is being deleted
    int32_t           broadCast;                        // This buffer was broadcast to all inputs
    int32_t       metaDataOnlyTransfer; // Only meta data transfered
    uint32_t      srcRank;              // rank of the output buffer
    int32_t       srcTemporalId;        // temporal buffer id
    BooleanToken           endOfWhole;           // end of whole data distribution
    uint32_t           nPartsPerWhole;                // Number of parts to make up the whole data set
    uint32_t           partsSequence;            // This buffers sequence in the whole
    BooleanToken           endOfStream;          // end of data stream
    int32_t           timeStamp;                        // Buffer time stamp
    DtOsDataTypes::Offset  localStateOffset;     // offset back to local state
    int32_t       outputSmbId;          // Output smb id
    BufferShape                   shape;                                // buffer shape
    uint32_t             zcopy;


  };




  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

}

#endif
