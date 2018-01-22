/* This is a managed file. Do not delete this comment. */

#include <driver/mnt/connext/shapes/shapes.h>
#include "ndds/ndds_c.h"
#include "ShapeType.h"
#include "ShapeTypeSupport.h"

static
void ShapeTypeExtendedListener_on_requested_deadline_missed(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_RequestedDeadlineMissedStatus *status)
{
    corto_error("deadline_missed");
}

static
void ShapeTypeExtendedListener_on_requested_incompatible_qos(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_RequestedIncompatibleQosStatus *status)
{
    corto_error("requested_incompatible_qos");
}

static
void ShapeTypeExtendedListener_on_sample_rejected(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SampleRejectedStatus *status)
{
    corto_error("sample_rejected");
}

static
void ShapeTypeExtendedListener_on_liveliness_changed(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_LivelinessChangedStatus *status)
{
    corto_info("liveliness_changed");
}

static
void ShapeTypeExtendedListener_on_sample_lost(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SampleLostStatus *status)
{
    corto_warning("sample_lost");
}

static
void ShapeTypeExtendedListener_on_subscription_matched(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SubscriptionMatchedStatus *status)
{
    corto_info("subscription_matched");
}

static
void ShapeTypeExtendedListener_on_data_available(
    void* listener_data,
    DDS_DataReader* reader)
{
    shapes_mount this = listener_data;

    ShapeTypeExtendedDataReader *ShapeTypeExtended_reader = NULL;
    struct ShapeTypeExtendedSeq data_seq = DDS_SEQUENCE_INITIALIZER;
    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    DDS_ReturnCode_t retcode;
    int i;

    ShapeTypeExtended_reader = ShapeTypeExtendedDataReader_narrow(reader);
    if (ShapeTypeExtended_reader == NULL) {
        fprintf(stderr, "DataReader narrow error\n");
        return;
    }

    retcode = ShapeTypeExtendedDataReader_take(
        ShapeTypeExtended_reader,
        &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
        DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
    if (retcode == DDS_RETCODE_NO_DATA) {
        return;
    } else if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "take error %d\n", retcode);
        return;
    }

    corto_object mount_root =
        corto_lookup(root_o, corto_subscriber(this)->query.from);

    for (i = 0; i < ShapeTypeExtendedSeq_get_length(&data_seq); ++i) {
        struct DDS_SampleInfo *info =
            DDS_SampleInfoSeq_get_reference(&info_seq, i);

        ShapeTypeExtended *sample =
            ShapeTypeExtendedSeq_get_reference(&data_seq, i);

        if (info->instance_state & DDS_NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
            /*corto_object o = corto_lookup(mount_root, sample->parent.color);
            corto_delete(o);
            corto_release(o);*/
        } else if (info->valid_data) {
            MyShapeType obj = corto_declare(
                mount_root, sample->parent.color, MyShapeType_o);
            if (corto_check_state(obj, CORTO_VALID)) {
                corto_update_begin(obj);
            }

            corto_set_str(&obj->shape, this->topic);
            corto_set_str(&obj->color, sample->parent.color);
            obj->x = sample->parent.x / 50.0 - 2.0;
            obj->y = -sample->parent.y / 50.0 + 3.0;
            obj->size = sample->parent.shapesize / 60.0;
            strlower(obj->shape);
            strlower(obj->color);

            if (corto_check_state(obj, CORTO_VALID)) {
                corto_update_end(obj);
            } else {
                corto_define(obj);
            }
            //printf("Received data [%d, %d, %s]\n", sample->parent.x, sample->parent.y, sample->parent.color);
        }
    }

    corto_release(mount_root);

    retcode = ShapeTypeExtendedDataReader_return_loan(
        ShapeTypeExtended_reader,
        &data_seq, &info_seq);
    if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "return loan error %d\n", retcode);
    }
}

/* Delete all entities */
static
int subscriber_shutdown(
    DDS_DomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = DDS_DomainParticipant_delete_contained_entities(participant);
        if (retcode != DDS_RETCODE_OK) {
            fprintf(stderr, "delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDS_DomainParticipantFactory_delete_participant(
            DDS_TheParticipantFactory, participant);
        if (retcode != DDS_RETCODE_OK) {
            fprintf(stderr, "delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    return status;
}

int16_t shapes_mount_construct(
    shapes_mount this)
{
    DDS_DomainParticipant *participant = NULL;
    DDS_Subscriber *subscriber = NULL;
    DDS_Topic *topic = NULL;
    struct DDS_DataReaderListener reader_listener =
    DDS_DataReaderListener_INITIALIZER;
    DDS_DataReader *reader = NULL;
    DDS_ReturnCode_t retcode;
    const char *type_name = NULL;

    if (!this->topic) {
        corto_set_str(&this->topic, "Square");
    }

    /* Make sure scope in query exists */
    corto_create(
        root_o,
        corto_subscriber(this)->query.from,
        corto_void_o);

    /* To customize participant QoS, use
    the configuration file USER_QOS_PROFILES.xml */
    participant = DDS_DomainParticipantFactory_create_participant(
        DDS_TheParticipantFactory, this->domainId, &DDS_PARTICIPANT_QOS_DEFAULT,
        NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        corto_throw("create_participant error");
        subscriber_shutdown(participant);
        goto error;
    }

    /* To customize subscriber QoS, use
    the configuration file USER_QOS_PROFILES.xml */
    subscriber = DDS_DomainParticipant_create_subscriber(
        participant, &DDS_SUBSCRIBER_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (subscriber == NULL) {
        corto_throw("create_subscriber error");
        subscriber_shutdown(participant);
        goto error;
    }

    /* Register the type before creating the topic */
    type_name = ShapeTypeExtendedTypeSupport_get_type_name();
    retcode = ShapeTypeExtendedTypeSupport_register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        corto_throw("register_type error %d", retcode);
        subscriber_shutdown(participant);
        goto error;
    }

    /* To customize topic QoS, use
    the configuration file USER_QOS_PROFILES.xml */
    topic = DDS_DomainParticipant_create_topic(
        participant, this->topic,
        type_name, &DDS_TOPIC_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        corto_throw("create_topic error");
        subscriber_shutdown(participant);
        goto error;
    }

    /* Set up a data reader listener */
    reader_listener.on_requested_deadline_missed  =
    ShapeTypeExtendedListener_on_requested_deadline_missed;
    reader_listener.on_requested_incompatible_qos =
    ShapeTypeExtendedListener_on_requested_incompatible_qos;
    reader_listener.on_sample_rejected =
    ShapeTypeExtendedListener_on_sample_rejected;
    reader_listener.on_liveliness_changed =
    ShapeTypeExtendedListener_on_liveliness_changed;
    reader_listener.on_sample_lost =
    ShapeTypeExtendedListener_on_sample_lost;
    reader_listener.on_subscription_matched =
    ShapeTypeExtendedListener_on_subscription_matched;
    reader_listener.on_data_available =
    ShapeTypeExtendedListener_on_data_available;
    reader_listener.as_listener.listener_data = this;

    /* To customize data reader QoS, use
    the configuration file USER_QOS_PROFILES.xml */
    reader = DDS_Subscriber_create_datareader(
        subscriber, DDS_Topic_as_topicdescription(topic),
        &DDS_DATAREADER_QOS_DEFAULT, &reader_listener, DDS_STATUS_MASK_ALL);
    if (reader == NULL) {
        corto_throw("create_datareader error");
        subscriber_shutdown(participant);
        goto error;
    }

    return 0;
error:
    return -1;
}
