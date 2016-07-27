#ifndef INVENTORY_H_
#define INVENTORY_H_

#include <mclib/Slot.h>
#include <mclib/Packets/PacketHandler.h>
#include <mclib/Connection.h>

class Inventory {
private:
    Minecraft::Connection* m_Connection;
    std::map<s32, Minecraft::Slot> m_Inventory;
    int m_WindowId;

public:
    Inventory(Minecraft::Connection* connection, int windowId);

    Minecraft::Slot* GetSlot(s32 index);

    // Sends a packet telling server to change the slot
    void SetSlot(s32 index, Minecraft::Slot slot);

    friend class InventoryManager;
};

class InventoryManager : public Minecraft::Packets::PacketHandler {
private:
    Minecraft::Connection* m_Connection;
    std::map<s32, std::shared_ptr<Inventory>> m_Inventories;

    void SetSlot(s32 windowId, s32 slotIndex, const Minecraft::Slot& slot);

public:
    InventoryManager(Minecraft::Packets::PacketDispatcher* dispatcher, Minecraft::Connection* connection);
    ~InventoryManager();

    void HandlePacket(Minecraft::Packets::Inbound::SetSlotPacket* packet);
    void HandlePacket(Minecraft::Packets::Inbound::WindowItemsPacket* packet);

    std::shared_ptr<Inventory> GetInventory(s32 windowId);
};

#endif
