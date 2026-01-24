-- Pouze definice promennych
local counter = 0

-- Kod bbezi jen kdyz ho C++ zavola
function rx(data)
    while counter < 10 do -- Kratka smycka uvnitr funkce je OK
       counter = counter + 1
    end
    return data
end