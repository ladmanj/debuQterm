-- Globální proměnná pro odpočet (0 = nic se neděje)
local dtr_reset_timer = 0

-- Definice maker
local MACROS = {
    ["F1"] = "HELP\n",
    
    -- F5: Hard Reset (pulz DTR)
    ["F5"] = function()
        print_log("--- HARD RESET START ---\r\n")
        print_log("DTR -> HIGH (Resetting...)\r\n")
        
        set_dtr(true)       -- "Zmáčkneme" reset
        dtr_reset_timer = 500 -- Nastavíme budík na 500 ms
    end
}

-- Zpracování kláves (voláno z C++ Event Filteru)
function on_key_down(key, mods)
    local action = MACROS[key]
    
    if action then
        if type(action) == "string" then
            send_raw(action)
        elseif type(action) == "function" then
            action() -- Spustíme funkci (která jen nastaví timer)
        end
        return true -- Klávesa zpracována
    end
    return false
end

-- Tikání (voláno každých X ms z C++)
function on_tick(delta_ms)
    -- Kontrola, zda běží odpočet pro reset
    if dtr_reset_timer > 0 then
        dtr_reset_timer = dtr_reset_timer - delta_ms
        
        -- Čas vypršel?
        if dtr_reset_timer <= 0 then
            dtr_reset_timer = 0 -- Vynulujeme, ať to neběží znovu
            
            set_dtr(false)       -- "Pustíme" reset
            print_log("DTR -> LOW (Done)\r\n")
            print_log("--- HARD RESET FINISH ---\r\n")
        end
    end
end
