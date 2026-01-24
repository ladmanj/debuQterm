-- =========================================================
--  RESPONSIVE HEX DUMP (Silent Mode)
-- =========================================================

-- --- KONFIGURACE ---
local CHARS_PER_BYTE = 3  -- "XX " = 3 znaky
local DEFAULT_WIDTH = 16  -- Fallback

-- --- STAVOVÉ PROMĚNNÉ ---
local col_counter = 0
local bytes_per_line = DEFAULT_WIDTH

-- Pomocná funkce pro výpočet limitu
local function recalc_limit(cols)
    if not cols then return DEFAULT_WIDTH end
    
    local w = math.floor(cols / CHARS_PER_BYTE)
    if w < 1 then w = 1 end 
    return w
end

-- 1. Inicializace (pokud C++ poslalo rozměry hned po startu)
if TERM_COLS then
    bytes_per_line = recalc_limit(TERM_COLS)
end

-- 2. Handler změny velikosti (volá C++ při resize)
function on_resize(rows, cols)
    -- Jen tiše přepočítáme limit pro příští data
    bytes_per_line = recalc_limit(cols)
    
    -- Vracíme prázdný string = do terminálu se nic nevypíše
    return "" 
end

-- 3. Zpracování příchozích dat
function rx(data)
    local result = {}
    
    for i = 1, #data do
        local byte = string.byte(data, i)
        
        -- Formát "FA "
        table.insert(result, string.format("%02X ", byte))
        
        col_counter = col_counter + 1
        
        -- Zalomit podle AKTUÁLNÍHO limitu
        if col_counter >= bytes_per_line then
            table.insert(result, "\r\n") 
            col_counter = 0
        end
    end

    return table.concat(result)
end

-- 4. Pass-through pro TX
function tx(data)
    return data
end
