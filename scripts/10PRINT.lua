local width = 20

if TERM_COLS then
    width = TERM_COLS
end

function on_resize(rows, cols)
    width = cols
end

function on_tick(delta)
    for i = 1,width do
        if math.random() >= 0.5 then
            print_log("╲")
        else
            print_log("╱")
        end
    end
    return nil
end
