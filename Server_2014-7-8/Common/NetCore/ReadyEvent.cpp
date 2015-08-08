#include "NativeFeatureIncludes.h"
#if _VENET_SUPPORT_ReadyEvent==1

#include "ReadyEvent.h"
#include "VEPeerInterface.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include "VEAssert.h"

#ifdef _MSC_VER
#pragma warning( push )
#endif

using namespace VENet;

int VENet::ReadyEvent::RemoteSystemCompBySystemAddress( const SystemAddress &key, const RemoteSystem &data )
{
    if (key < data.systemAddress)
        return -1;
    else if (key==data.systemAddress)
        return 0;
    else
        return 1;
}

int VENet::ReadyEvent::ReadyEventNodeComp( const int &key, ReadyEvent::ReadyEventNode * const &data )
{
    if (key < data->eventId)
        return -1;
    else if (key==data->eventId)
        return 0;
    else
        return 1;
}

STATIC_FACTORY_DEFINITIONS(ReadyEvent,ReadyEvent);

ReadyEvent::ReadyEvent()
{
    channel=0;
}

ReadyEvent::~ReadyEvent()
{
    Clear();
}


bool ReadyEvent::SetEvent(int eventId, bool isReady)
{
    bool objectExists;
    unsigned eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists==false)
    {
        // Totally new event
        CreateNewEvent(eventId, isReady);
    }
    else
    {
        return SetEventByIndex(eventIndex, isReady);
    }
    return true;
}
void ReadyEvent::ForceCompletion(int eventId)
{
    bool objectExists;
    unsigned eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists==false)
    {
        // Totally new event
        CreateNewEvent(eventId, true);
        eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    }

    ReadyEventNode *ren = readyEventNodeList[eventIndex];
    ren->eventStatus=ID_READY_EVENT_FORCE_ALL_SET;
    UpdateReadyStatus(eventIndex);
}
bool ReadyEvent::DeleteEvent(int eventId)
{
    bool objectExists;
    unsigned eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        VENet::OP_DELETE(readyEventNodeList[eventIndex], _FILE_AND_LINE_);
        readyEventNodeList.RemoveAtIndex(eventIndex);
        return true;
    }
    return false;
}
bool ReadyEvent::IsEventSet(int eventId)
{
    bool objectExists;
    unsigned eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        return readyEventNodeList[eventIndex]->eventStatus==ID_READY_EVENT_SET || readyEventNodeList[eventIndex]->eventStatus==ID_READY_EVENT_ALL_SET;
    }
    return false;
}
bool ReadyEvent::IsEventCompletionProcessing(int eventId) const
{
    bool objectExists;
    unsigned eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        bool anyAllReady=false;
        bool allAllReady=true;
        ReadyEventNode *ren = readyEventNodeList[eventIndex];
        if (ren->eventStatus==ID_READY_EVENT_FORCE_ALL_SET)
            return false;
        for (unsigned i=0; i < ren->systemList.Size(); i++)
        {
            if (ren->systemList[i].lastReceivedStatus==ID_READY_EVENT_ALL_SET)
                anyAllReady=true;
            else
                allAllReady=false;
        }
        return anyAllReady==true && allAllReady==false;
    }
    return false;
}
bool ReadyEvent::IsEventCompleted(int eventId) const
{
    bool objectExists;
    unsigned eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        return IsEventCompletedByIndex(eventIndex);
    }
    return false;
}

bool ReadyEvent::HasEvent(int eventId)
{
    return readyEventNodeList.HasData(eventId);
}

unsigned ReadyEvent::GetEventListSize(void) const
{
    return readyEventNodeList.Size();
}

int ReadyEvent::GetEventAtIndex(unsigned index) const
{
    return readyEventNodeList[index]->eventId;
}

bool ReadyEvent::AddToWaitList(int eventId, SystemAddress address)
{
    bool eventExists;
    unsigned eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &eventExists);
    if (eventExists==false)
        eventIndex=CreateNewEvent(eventId, false);

    unsigned i;
    unsigned numAdded=0;
    if (address==UNASSIGNED_SYSTEM_ADDRESS)
    {
        for (i=0; i < vePeerInterface->GetMaximumNumberOfPeers(); i++)
        {
            SystemAddress internalAddress = vePeerInterface->GetSystemAddressFromIndex(i);
            if (internalAddress!=UNASSIGNED_SYSTEM_ADDRESS)
            {
                numAdded+=AddToWaitListInternal(eventIndex, internalAddress);
            }
        }
    }
    else
    {
        numAdded=AddToWaitListInternal(eventIndex, address);
    }

    if (numAdded>0)
        UpdateReadyStatus(eventIndex);
    return numAdded>0;
}
bool ReadyEvent::RemoveFromWaitList(int eventId, SystemAddress address)
{
    bool eventExists;
    unsigned eventIndex = readyEventNodeList.GetIndexFromKey(eventId, &eventExists);
    if (eventExists)
    {
        if (address==UNASSIGNED_SYSTEM_ADDRESS)
        {
            readyEventNodeList[eventIndex]->systemList.Clear(false, _FILE_AND_LINE_);
            UpdateReadyStatus(eventIndex);
        }
        else
        {
            bool systemExists;
            unsigned systemIndex = readyEventNodeList[eventIndex]->systemList.GetIndexFromKey(address, &systemExists);
            if (systemExists)
            {
                bool isCompleted = IsEventCompletedByIndex(eventIndex);
                readyEventNodeList[eventIndex]->systemList.RemoveAtIndex(systemIndex);

                if (isCompleted==false && IsEventCompletedByIndex(eventIndex))
                    PushCompletionPacket(readyEventNodeList[eventIndex]->eventId);

                UpdateReadyStatus(eventIndex);

                return true;
            }
        }
    }

    return false;
}
bool ReadyEvent::IsInWaitList(int eventId, SystemAddress address)
{
    bool objectExists;
    unsigned readyIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        return readyEventNodeList[readyIndex]->systemList.HasData(address);
    }
    return false;
}

unsigned ReadyEvent::GetRemoteWaitListSize(int eventId) const
{
    bool objectExists;
    unsigned readyIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        return readyEventNodeList[readyIndex]->systemList.Size();
    }
    return 0;
}

SystemAddress ReadyEvent::GetFromWaitListAtIndex(int eventId, unsigned index) const
{
    bool objectExists;
    unsigned readyIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        return readyEventNodeList[readyIndex]->systemList[index].systemAddress;
    }
    return UNASSIGNED_SYSTEM_ADDRESS;
}
ReadyEventSystemStatus ReadyEvent::GetReadyStatus(int eventId, SystemAddress address)
{
    bool objectExists;
    unsigned readyIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        ReadyEventNode *ren = readyEventNodeList[readyIndex];
        unsigned systemIndex = ren->systemList.GetIndexFromKey(address, &objectExists);
        if (objectExists==false)
            return RES_NOT_WAITING;
        if (ren->systemList[systemIndex].lastReceivedStatus==ID_READY_EVENT_SET)
            return RES_READY;
        if (ren->systemList[systemIndex].lastReceivedStatus==ID_READY_EVENT_UNSET)
            return RES_WAITING;
        if (ren->systemList[systemIndex].lastReceivedStatus==ID_READY_EVENT_ALL_SET)
            return RES_ALL_READY;
    }

    return RES_UNKNOWN_EVENT;
}
void ReadyEvent::SetSendChannel(unsigned char newChannel)
{
    channel=newChannel;
}
PluginReceiveResult ReadyEvent::OnReceive(Packet *packet)
{
    unsigned char packetIdentifier;
    packetIdentifier = ( unsigned char ) packet->data[ 0 ];

    switch (packetIdentifier)
    {
    case ID_READY_EVENT_UNSET:
    case ID_READY_EVENT_SET:
    case ID_READY_EVENT_ALL_SET:
        OnReadyEventPacketUpdate(packet);
        return RR_CONTINUE_PROCESSING;
    case ID_READY_EVENT_FORCE_ALL_SET:
        OnReadyEventForceAllSet(packet);
        return RR_CONTINUE_PROCESSING;
    case ID_READY_EVENT_QUERY:
        OnReadyEventQuery(packet);
        return RR_STOP_PROCESSING_AND_DEALLOCATE;
    }

    return RR_CONTINUE_PROCESSING;
}
bool ReadyEvent::AddToWaitListInternal(unsigned eventIndex, SystemAddress address)
{
    ReadyEventNode *ren = readyEventNodeList[eventIndex];
    bool objectExists;
    unsigned systemIndex = ren->systemList.GetIndexFromKey(address, &objectExists);
    if (objectExists==false)
    {
        RemoteSystem rs;
        rs.lastReceivedStatus=ID_READY_EVENT_UNSET;
        rs.lastSentStatus=ID_READY_EVENT_UNSET;
        rs.systemAddress=address;
        ren->systemList.InsertAtIndex(rs,systemIndex, _FILE_AND_LINE_);

        SendReadyStateQuery(ren->eventId, address);
        return true;
    }
    return false;
}
void ReadyEvent::OnReadyEventForceAllSet(Packet *packet)
{
    VENet::BitStream incomingBitStream(packet->data, packet->length, false);
    incomingBitStream.IgnoreBits(8);
    int eventId;
    incomingBitStream.Read(eventId);
    bool objectExists;
    unsigned readyIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        ReadyEventNode *ren = readyEventNodeList[readyIndex];
        if (ren->eventStatus!=ID_READY_EVENT_FORCE_ALL_SET)
        {
            ren->eventStatus=ID_READY_EVENT_FORCE_ALL_SET;
            PushCompletionPacket(ren->eventId);
        }
    }
}
void ReadyEvent::OnReadyEventPacketUpdate(Packet *packet)
{
    VENet::BitStream incomingBitStream(packet->data, packet->length, false);
    incomingBitStream.IgnoreBits(8);
    int eventId;
    incomingBitStream.Read(eventId);
    bool objectExists;
    unsigned readyIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        ReadyEventNode *ren = readyEventNodeList[readyIndex];
        bool systemExists;
        unsigned systemIndex = ren->systemList.GetIndexFromKey(packet->systemAddress, &systemExists);
        if (systemExists)
        {
            if (ren->systemList[systemIndex].lastReceivedStatus==packet->data[0])
                return;

            bool wasCompleted = IsEventCompletedByIndex(readyIndex);
            ren->systemList[systemIndex].lastReceivedStatus=packet->data[0];
            if (ren->eventStatus==ID_READY_EVENT_FORCE_ALL_SET)
                return;
            UpdateReadyStatus(readyIndex);
            if (wasCompleted==false && IsEventCompletedByIndex(readyIndex))
                PushCompletionPacket(readyIndex);
        }
    }
}
void ReadyEvent::OnReadyEventQuery(Packet *packet)
{
    VENet::BitStream incomingBitStream(packet->data, packet->length, false);
    incomingBitStream.IgnoreBits(8);
    int eventId;
    incomingBitStream.Read(eventId);
    bool objectExists;
    unsigned readyIndex = readyEventNodeList.GetIndexFromKey(eventId, &objectExists);
    if (objectExists)
    {
        unsigned systemIndex = readyEventNodeList[readyIndex]->systemList.GetIndexFromKey(packet->systemAddress,&objectExists);
        if (objectExists)
            SendReadyUpdate(readyIndex, systemIndex, true);
    }
}
void ReadyEvent::OnClosedConnection(const SystemAddress &systemAddress, VENetGUID VENetGuid, PI2_LostConnectionReason lostConnectionReason )
{
    (void) systemAddress;
    (void) VENetGuid;
    (void) lostConnectionReason;

    RemoveFromAllLists(systemAddress);
}
void ReadyEvent::OnRakPeerShutdown(void)
{
    Clear();
}

bool ReadyEvent::SetEventByIndex(int eventIndex, bool isReady)
{
    ReadyEventNode *ren = readyEventNodeList[eventIndex];
    if ((ren->eventStatus==ID_READY_EVENT_ALL_SET || ren->eventStatus==ID_READY_EVENT_SET) && isReady==true)
        return false;
    if (ren->eventStatus==ID_READY_EVENT_UNSET && isReady==false)
        return false;
    if (ren->eventStatus==ID_READY_EVENT_FORCE_ALL_SET)
        return false;

    if (isReady)
        ren->eventStatus=ID_READY_EVENT_SET;
    else
        ren->eventStatus=ID_READY_EVENT_UNSET;

    UpdateReadyStatus(eventIndex);

    if (IsEventCompletedByIndex(eventIndex))
    {
        PushCompletionPacket(ren->eventId);
    }

    return true;
}

bool ReadyEvent::IsEventCompletedByIndex(unsigned eventIndex) const
{
    ReadyEventNode *ren = readyEventNodeList[eventIndex];
    unsigned i;
    if (ren->eventStatus==ID_READY_EVENT_FORCE_ALL_SET)
        return true;
    if (ren->eventStatus!=ID_READY_EVENT_ALL_SET)
        return false;
    for (i=0; i < ren->systemList.Size(); i++)
        if (ren->systemList[i].lastReceivedStatus!=ID_READY_EVENT_ALL_SET)
            return false;
    return true;
}

void ReadyEvent::Clear(void)
{
    unsigned i;
    for (i=0; i < readyEventNodeList.Size(); i++)
    {
        VENet::OP_DELETE(readyEventNodeList[i], _FILE_AND_LINE_);
    }
    readyEventNodeList.Clear(false, _FILE_AND_LINE_);
}

unsigned ReadyEvent::CreateNewEvent(int eventId, bool isReady)
{
    ReadyEventNode *ren = VENet::OP_NEW<ReadyEventNode>( _FILE_AND_LINE_ );
    ren->eventId=eventId;
    if (isReady==false)
        ren->eventStatus=ID_READY_EVENT_UNSET;
    else
        ren->eventStatus=ID_READY_EVENT_SET;
    return readyEventNodeList.Insert(eventId, ren, true, _FILE_AND_LINE_);
}
void ReadyEvent::UpdateReadyStatus(unsigned eventIndex)
{
    ReadyEventNode *ren = readyEventNodeList[eventIndex];
    bool anyUnset;
    unsigned i;
    if (ren->eventStatus==ID_READY_EVENT_SET)
    {
        anyUnset=false;
        for (i=0; i < ren->systemList.Size(); i++)
        {
            if (ren->systemList[i].lastReceivedStatus==ID_READY_EVENT_UNSET)
            {
                anyUnset=true;
                break;
            }
        }
        if (anyUnset==false)
        {
            ren->eventStatus=ID_READY_EVENT_ALL_SET;
        }
    }
    else if (ren->eventStatus==ID_READY_EVENT_ALL_SET)
    {
        anyUnset=false;
        for (i=0; i < ren->systemList.Size(); i++)
        {
            if (ren->systemList[i].lastReceivedStatus==ID_READY_EVENT_UNSET)
            {
                anyUnset=true;
                break;
            }
        }
        if (anyUnset==true)
        {
            ren->eventStatus=ID_READY_EVENT_SET;
        }
    }
    BroadcastReadyUpdate(eventIndex, false);
}
void ReadyEvent::SendReadyUpdate(unsigned eventIndex, unsigned systemIndex, bool forceIfNotDefault)
{
    ReadyEventNode *ren = readyEventNodeList[eventIndex];
    VENet::BitStream bs;
    if ((ren->eventStatus!=ren->systemList[systemIndex].lastSentStatus) ||
            (forceIfNotDefault && ren->eventStatus!=ID_READY_EVENT_UNSET))
    {
        bs.Write(ren->eventStatus);
        bs.Write(ren->eventId);
        SendUnified(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, channel, ren->systemList[systemIndex].systemAddress, false);

        ren->systemList[systemIndex].lastSentStatus=ren->eventStatus;
    }

}
void ReadyEvent::BroadcastReadyUpdate(unsigned eventIndex, bool forceIfNotDefault)
{
    ReadyEventNode *ren = readyEventNodeList[eventIndex];
    unsigned systemIndex;
    for (systemIndex=0; systemIndex < ren->systemList.Size(); systemIndex++)
    {
        SendReadyUpdate(eventIndex, systemIndex, forceIfNotDefault);
    }
}
void ReadyEvent::SendReadyStateQuery(unsigned eventId, SystemAddress address)
{
    VENet::BitStream bs;
    bs.Write((MessageID)ID_READY_EVENT_QUERY);
    bs.Write(eventId);
    SendUnified(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, channel, address, false);
}
void ReadyEvent::RemoveFromAllLists(SystemAddress address)
{
    unsigned eventIndex;
    for (eventIndex=0; eventIndex < readyEventNodeList.Size(); eventIndex++)
    {
        bool isCompleted = IsEventCompletedByIndex(eventIndex);
        bool systemExists;
        unsigned systemIndex;

        systemIndex = readyEventNodeList[eventIndex]->systemList.GetIndexFromKey(address, &systemExists);
        if (systemExists)
            readyEventNodeList[eventIndex]->systemList.RemoveAtIndex(systemIndex);

        UpdateReadyStatus(eventIndex);

        if (isCompleted==false && IsEventCompletedByIndex(eventIndex))
            PushCompletionPacket(readyEventNodeList[eventIndex]->eventId);
    }
}
void ReadyEvent::PushCompletionPacket(unsigned eventId)
{
    (void) eventId;
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif