#include "PlayerBroadcaster.h"
#include "MovementBroadcaster.h"
#include "World.h"
#include "Player.h"

uint32 PlayerBroadcaster::num_bcaster_created = 0;
uint32 PlayerBroadcaster::num_bcaster_deleted = 0;

PlayerBroadcaster::PlayerBroadcaster(WorldSocket* w_socket, const ObjectGuid& self, std::size_t max_queue) :
    MAX_QUEUE_SIZE(max_queue), m_socket(w_socket), m_self(self), instanceId(0), lastUpdatePackets(0)
{
    if (m_socket)
        m_socket->AddReference();

    m_queue.reserve(max_queue);
    ++num_bcaster_created;
}

void PlayerBroadcaster::ChangeSocket(WorldSocket* new_socket)
{
    if (m_socket)
        m_socket->RemoveReference();

    if (new_socket)
        new_socket->AddReference();

    m_socket = new_socket;
}

bool PlayerBroadcaster::BeginProcessing()
{
    std::lock_guard<std::mutex> guard(m_processing_lock);
    if (!m_processing_enabled)
        return false;

    ++m_active_processing;
    return true;
}

void PlayerBroadcaster::FinishProcessing()
{
    std::lock_guard<std::mutex> guard(m_processing_lock);
    ASSERT(m_active_processing > 0);
    if (--m_active_processing == 0)
        m_processing_idle.notify_all();
}

void PlayerBroadcaster::StopProcessing()
{
    std::unique_lock<std::mutex> guard(m_processing_lock);
    m_processing_enabled = false;
    m_processing_idle.wait(guard, [this]() { return m_active_processing == 0; });
}

void PlayerBroadcaster::WaitForListenerBatches(std::unique_lock<std::mutex>& lock)
{
    m_listeners_idle.wait(lock, [this]() { return m_active_listener_batches == 0; });
}

void PlayerBroadcaster::FinishListenerBatch()
{
    std::lock_guard<std::mutex> guard(m_listeners_lock);
    ASSERT(m_active_listener_batches > 0);
    if (--m_active_listener_batches == 0)
        m_listeners_idle.notify_all();
}

void PlayerBroadcaster::AddListener(Player const* player)
{
    ASSERT(player);
    if (player->GetObjectGuid() == m_self)
        return;

    std::unique_lock<std::mutex> guard(m_listeners_lock);
    WaitForListenerBatches(guard);
    m_listeners[player->GetObjectGuid()] = player->m_broadcaster;
}

void PlayerBroadcaster::RemoveListener(Player const* player)
{
    ASSERT(player);
    std::unique_lock<std::mutex> guard(m_listeners_lock);
    WaitForListenerBatches(guard);
    m_listeners.erase(player->GetObjectGuid());
}

void PlayerBroadcaster::ClearListeners()
{
    std::unique_lock<std::mutex> guard(m_listeners_lock);
    WaitForListenerBatches(guard);
    m_listeners.clear();
}

void PlayerBroadcaster::SendPacket(const WorldPacket& packet)
{
    if (m_socket)
        m_socket->SendPacket(packet);
}

void PlayerBroadcaster::ProcessQueue(uint32& num_packets)
{
    if (!BeginProcessing())
        return;

    ProcessingGuard processingGuard(*this);

    std::vector<BroadcastData> queue;
    {
        std::lock_guard<std::mutex> guard(m_queue_lock);
        if (m_queue.empty())
            return;

        queue.swap(m_queue);
    }

    std::vector<std::pair<ObjectGuid, std::shared_ptr<PlayerBroadcaster>>> listeners;
    {
        std::lock_guard<std::mutex> guard(m_listeners_lock);
        listeners.reserve(m_listeners.size());
        listeners.insert(listeners.end(), m_listeners.begin(), m_listeners.end());
        ++m_active_listener_batches;
    }

    lastUpdatePackets = queue.size() * listeners.size();
    num_packets += lastUpdatePackets;

    try
    {
        for (auto& data : queue)
        {
            if (data.sendToSelf && data.except != GetGUID())
                SendPacket(data.packet);

            for (const auto& itr : listeners)
            {
                if (itr.first == data.except)
                    continue;

                itr.second->SendPacket(data.packet);
            }
        }
    }
    catch (...)
    {
        FinishListenerBatch();
        throw;
    }

    FinishListenerBatch();
    queue.clear();
    std::lock_guard<std::mutex> guard(m_queue_lock);
    if (m_queue.empty())
        m_queue.swap(queue);
}

void PlayerBroadcaster::QueuePacket(WorldPacket packet, bool self, ObjectGuid except)
{
    BroadcastData data;
    data.packet = std::move(packet);
    data.sendToSelf = self;
    data.except = except;

    std::scoped_lock guard(m_queue_lock);

    // We need to drop a packet here - if possible
    if (m_queue.size() >= MAX_QUEUE_SIZE)
    {
        BroadcastData& last_in_queue = m_queue[m_queue.size() - 1];
        if (CanSkipPacket(last_in_queue.packet.GetOpcode()) && CanSkipPacket(data.packet.GetOpcode()))
        {
            m_queue[m_queue.size() - 1] = std::move(data);
            return;
        }
    }

    m_queue.emplace_back(std::move(data));
}

ObjectGuid PlayerBroadcaster::GetGUID() const
{
    return m_self;
}

void PlayerBroadcaster::FreeAtLogout()
{
    StopProcessing();

    if (m_socket)
    {
        m_socket->RemoveReference();
        m_socket = nullptr;
    }

    const std::scoped_lock lock{ m_queue_lock, m_listeners_lock };
    m_queue.clear();
    m_listeners.clear();
    
}

PlayerBroadcaster::~PlayerBroadcaster()
{
    if (m_socket)
        m_socket->RemoveReference();

    ++num_bcaster_deleted;
}
