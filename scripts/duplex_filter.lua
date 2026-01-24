-- Příklad: protocol_wrapper.lua

-- 1. Zpracování PŘÍJMU (ze zařízení do PC)
function rx(data)
    local result = ""
    for i = 1, #data do
        local b = string.byte(data, i)
        -- Zobrazíme jako hex [XX]
        result = result .. string.format("[%02X] ", b)
    end
    result = result .. "\n"
    return result
end

-- 2. Zpracování VYSÍLÁNÍ (z PC do zařízení)
function tx(data)
    -- Představme si, že posíláme příkaz
    -- Zařízení vyžaduje formát: <STX> data <ETX>
    
    local STX = string.char(0x02)
    local ETX = string.char(0x03)
    
    -- Vezmeme vstup z klávesnice a obalíme ho
    return STX .. data .. ETX
end