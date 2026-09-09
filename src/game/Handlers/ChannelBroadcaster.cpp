#include "ChannelBroadcaster.h"
#include "ChannelMgr.h"
#include "World.h"


ChannelBroadcaster::ChannelBroadcaster() : MessageQueue(15)
{
	StartThread();
}

ChannelBroadcaster::~ChannelBroadcaster()
{
	Stop();
}

void ChannelBroadcaster::StartThread()
{
	Worker = new std::thread([this]()
	{
		ThreadProc();
	});
}

void ChannelBroadcaster::Stop()
{
	if (Worker == nullptr)
	{
		return;
	}

    {
        std::lock_guard<std::mutex> lock(StateMutex);
        bStopRequested = true;
        bShouldSentMessages = false;
    }
    StateChanged.notify_all();
	
	if (Worker->joinable())
	{
		Worker->join();
	}

	delete Worker;
	Worker = nullptr;
}

void ChannelBroadcaster::EnableSendingMessages()
{
    std::unique_lock<std::mutex> lock(StateMutex);
    bShouldSentMessages = true;
    StateChanged.notify_all();
    StateChanged.wait(lock, [this]()
	{
        return bIsWorking || bStopRequested || sWorld.IsStopped();
    });
}

void ChannelBroadcaster::DisableSendingMessages()
{
    std::unique_lock<std::mutex> lock(StateMutex);
    bShouldSentMessages = false;
    StateChanged.notify_all();
    StateChanged.wait(lock, [this]()
	{
        return !bIsWorking;
    });
}

void ChannelBroadcaster::EnqueueMessage(std::string&& Message, const std::string& ChannelName, ObjectGuid PlayerGuid, uint32 Language, Team ChannelTeam, bool bSkipChecks)
{
    // ReaderWriterQueue is SPSC. AsyncSay can be called by multiple producer
    // threads, so serialize producer access while also synchronizing the queue
    // transition with the consumer's empty-queue wait predicate.
    {
        std::lock_guard<std::mutex> lock(StateMutex);
        MessageQueue.enqueue(ChannelMessage{std::move(Message), ChannelName, PlayerGuid, Language, ChannelTeam, bSkipChecks });
    }
    StateChanged.notify_one();
}

void ChannelBroadcaster::ThreadProc()
{
    for (;;)
	{
		{
            std::unique_lock<std::mutex> lock(StateMutex);
            StateChanged.wait(lock, [this]()
            {
                return bShouldSentMessages || bStopRequested || sWorld.IsStopped();
            });

            if (bStopRequested || sWorld.IsStopped())
                break;

            bIsWorking = true;
            StateChanged.notify_all();
        }

        for (;;)
        {
            {
                std::lock_guard<std::mutex> lock(StateMutex);
                if (!bShouldSentMessages || bStopRequested || sWorld.IsStopped())
                    break;
            }

			constexpr int32 MessageLimit = 5;
			int32 MessageIterator = 0;

			ChannelMessage msg;
            while (MessageIterator < MessageLimit && MessageQueue.try_dequeue(msg))
			{
                ChannelMgr* ChannelManager = channelMgr(msg.ChannelTeam);
                Channel* TargetChannel = ChannelManager->GetOrCreateChannel(msg.ChannelName);
                TargetChannel->Say(msg.PlayerGuid, msg.Message.c_str(), msg.Language, msg.bSkipChecks);
                ++MessageIterator;
            }

            if (MessageIterator == 0)
            {
                std::unique_lock<std::mutex> lock(StateMutex);
                StateChanged.wait(lock, [this]()
                {
                    return MessageQueue.peek() != nullptr || !bShouldSentMessages ||
                           bStopRequested || sWorld.IsStopped();
                });
            }
		}

        {
            std::lock_guard<std::mutex> lock(StateMutex);
            bIsWorking = false;
        }
        StateChanged.notify_all();
    }

    {
        std::lock_guard<std::mutex> lock(StateMutex);
        bIsWorking = false;
	}
    StateChanged.notify_all();
}
