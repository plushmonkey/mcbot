#ifndef INVENTORY_H_
#define INVENTORY_H_

#include <mclib/Slot.h>
#include <mclib/Packets/PacketHandler.h>
#include <mclib/Connection.h>

class Inventory {
public:
    static const s32 HOTBAR_SLOT_START;
    static const s32 PLAYER_INVENTORY_ID;

private:
    Minecraft::Connection* m_Connection;
    std::map<s32, Minecraft::Slot> m_Inventory;
    int m_WindowId;
    s32 m_SelectedHotbarIndex;

public:
    Inventory(Minecraft::Connection* connection, int windowId);

    Minecraft::Slot* GetSlot(s32 index);

    // Returns item slot index
    s32 FindItemById(s32 itemId);

    s32 GetSelectedHotbarSlot() const { return m_SelectedHotbarIndex; }

    // Select the hotbar slot to use. 0-8
    void SelectHotbarSlot(s32 hotbarIndex);

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
    void HandlePacket(Minecraft::Packets::Inbound::HeldItemChangePacket* packet);

    std::shared_ptr<Inventory> GetInventory(s32 windowId);
};

#endif
