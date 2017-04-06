#ifndef INVENTORY_H_
#define INVENTORY_H_

#include <mclib/core/Connection.h>
#include <mclib/inventory/Slot.h>
#include <mclib/protocol/packets/PacketHandler.h>

class Inventory {
public:
    static const s32 HOTBAR_SLOT_START;
    static const s32 PLAYER_INVENTORY_ID;

private:
    mc::core::Connection* m_Connection;
    std::map<s32, mc::inventory::Slot> m_Inventory;
    int m_WindowId;
    s32 m_SelectedHotbarIndex;

public:
    Inventory(mc::core::Connection* connection, int windowId);

    mc::inventory::Slot* GetSlot(s32 index);

    // Returns item slot index
    s32 FindItemById(s32 itemId);

    s32 GetSelectedHotbarSlot() const { return m_SelectedHotbarIndex; }

    // Select the hotbar slot to use. 0-8
    void SelectHotbarSlot(s32 hotbarIndex);

    friend class InventoryManager;
};

class InventoryManager : public mc::protocol::packets::PacketHandler {
private:
    mc::core::Connection* m_Connection;
    std::map<s32, std::shared_ptr<Inventory>> m_Inventories;

    void SetSlot(s32 windowId, s32 slotIndex, const mc::inventory::Slot& slot);

public:
    InventoryManager(mc::protocol::packets::PacketDispatcher* dispatcher, mc::core::Connection* connection);
    ~InventoryManager();

    void HandlePacket(mc::protocol::packets::in::SetSlotPacket* packet);
    void HandlePacket(mc::protocol::packets::in::WindowItemsPacket* packet);
    void HandlePacket(mc::protocol::packets::in::HeldItemChangePacket* packet);

    std::shared_ptr<Inventory> GetInventory(s32 windowId);
};

#endif
