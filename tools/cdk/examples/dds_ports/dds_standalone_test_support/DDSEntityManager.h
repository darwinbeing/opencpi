
/*
 *                         OpenSplice DDS
 *
 *   This software and documentation are Copyright 2006 to 2009 PrismTech
 *   Limited and its licensees. All rights reserved. See file:
 *
 *                     $OSPL_HOME/LICENSE
 *
 *   for full copyright notice and license terms.
 *
 */

#ifndef _DDSENTITYMGR_
  #define _DDSENTITYMGR_

  #include "ccpp_dds_dcps.h"
  #include <string>

  using namespace DDS;
  using namespace std;


  /**
   * Returns the name of an error code.
   **/
  string getErrorName(DDS::ReturnCode_t status);

  /**
   * Check the return status for errors. If there is an error, then terminate.
   **/
  void checkStatus(DDS::ReturnCode_t status, const char *info);

  /**
   * Check whether a valid handle has been returned. If not, then terminate.
   **/
  void checkHandle(void *handle, string info);


  class DDSEntityManager
  {

      /* Generic DDS entities */
      DomainParticipantFactory_var dpf;
      DomainParticipant_var participant;
      Topic_var topic;
      Publisher_var publisher;
      Subscriber_var subscriber;
      DataWriter_ptr writer;
      DataReader_ptr reader;

      /* QosPolicy holders */
      TopicQos reliable_topic_qos;
      TopicQos setting_topic_qos;
      PublisherQos pub_qos;
      DataWriterQos dw_qos;
      SubscriberQos sub_qos;

      DomainId_t domain;
      InstanceHandle_t userHandle;
      ReturnCode_t status;

      CORBA::String_var partition;
      CORBA::String_var typeName;
    public:
      void createParticipant(const char *partitiontName);
      void deleteParticipant();
      void registerType(TypeSupport *ts);
      void createTopic(char *topicName);
      void deleteTopic();
      void createPublisher();
      void deletePublisher();
      void createWriter();
	  void createWriter(bool autodispose_unregistered_instances);
	  void deleteWriter();
      void createSubscriber();
      void deleteSubscriber();
      void createReader();
      void deleteReader();
      DataReader_ptr getReader();
      DataWriter_ptr getWriter();
      Publisher_ptr getPublisher();
      Subscriber_ptr getSubscriber();
      Topic_ptr getTopic();
      DomainParticipant_ptr getParticipant();
      ~DDSEntityManager();
  };

#endif