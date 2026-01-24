local counter = 0
local LOADING_ANIM = {"|", "/", "-", "\\"}
local anim_idx = 1

function on_tick(delta_ms)
    counter = counter + delta_ms

    -- Každých 500 ms aktualizujeme animaci ve Status Baru
    if counter >= 500 then
        counter = 0
        
        -- Voláme C++ funkci (nevypisuje se do sériáku!)
        local frame = LOADING_ANIM[anim_idx]
        print_status("Waiting for device... " .. frame, 1000)
        
        anim_idx = anim_idx + 1
        if anim_idx > #LOADING_ANIM then anim_idx = 1 end
    end
    
    -- Návratová hodnota on_tick jde stále na SÉRIOVÝ PORT
    -- Pokud vrátíme nil, na port se nic nepošle.
    return nil 
end

function rx(data)
    return data
end
