-- Stavová proměnná pro čas
local timer_accumulator = 0
local INTERVAL = 5000 -- 5 sekund

-- Funkce volaná každých 100ms (nebo jak je nastaven C++ timer)
function on_tick(delta_ms)
    timer_accumulator = timer_accumulator + delta_ms
    
    if timer_accumulator >= INTERVAL then
        timer_accumulator = 0 -- Reset
        
        -- Vrátíme string -> C++ ho pošle na sériový port
        return "PING\n"
    end
    
    return nil -- Nic neposílat
end

-- Pass-through pro RX a TX
function rx(d) return d end
function tx(d) return d end
