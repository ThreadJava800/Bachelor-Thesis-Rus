typedef union {
    struct {
        uint32_t ReservedPos;
        uint32_t WriterCnt;
    } S;
    uint64_t U;
} SHADOW_W_POS;

typedef struct {
    uint32_t ReadPos;
    uint32_t WritePos;
    uint64_t IsOverflow;
    SHADOW_W_POS ShWPos;
    char Buffer[0]; 
} LOCKLESS_RING_BUFFER;

uint32_t LrbReserve (
    LOCKLESS_RING_BUFFER *Rb,
    const uint32_t MsgSize,
    SHADOW_W_POS *CurrShadowPos
) {
    for (;;) {
        Curr->U = Rb->ShWPos.U;
        SHADOW_W_POS NextShadowPos = {
            .S.ReservedPos = Curr->S.ReservedPos + MsgSize,
            .S.WritePos = Curr->S.WriterCnt + 1
        };

        if (atomic_compare_exchange(&Rb->ShWPos, &Curr->U, Next.U)) {
            // SHADOW_W_POS была обновлена в буфере
            // Таким образом, гарантируется, что ни один другой поток
            // не будет копировать своё сообщение в занятую память.
            return Curr->S.ReservedPos;
        }
    }
}

uint32_t LrbReserve (
    LOCKLESS_RING_BUFFER *Rb,
    const uint32_t MsgSize,
    SHADOW_W_POS *Curr
) {
    for (;;) {
        Curr->U = Rb->ShWPos.U;
        SHADOW_W_POS Next = {
            .S.ReservedPos = Curr->S.ReservedPos + MsgSize,
            .S.WritePos = Curr->S.WriterCnt + 1
        };

        if (atomic_compare_exchange(&Rb->ShWPos, &Curr->U, Next.U)) {
            // SHADOW_W_POS была обновлена в буфере
            // Таким образом, гарантируется, что ни один другой поток
            // не будет копировать своё сообщение в занятую память.
            return Curr->S.ReservedPos;
        }
    }
}

uint32_t LrbCommit (
    LOCKLESS_RING_BUFFER *Rb,
    SHADOW_W_POS *Curr
) {
    for (;;) {
        SHADOW_W_POS NextShadowPos = {
            .S.ReservedPos = Curr->S.ReservedPos,
            .S.WritePos = Curr->S.WriterCnt - 1
        };

        if (NextShadowPos.S.WriterCnt == 0) {
            // все сообщения до текущего закончили копирование
            Rb->WritePos = Curr->S.ReservedPos;
        }
        if (atomic_compare_exchange(&Rb->ShWPos, &Curr->U, Next.U)) {
            return;
        }
    }
}

uint32_t LrbReserve (
    LOCKLESS_RING_BUFFER *Rb,
    const uint32_t MsgSize,
    SHADOW_W_POS *Curr
) {
    for (;;) {
        Curr->U = Rb->ShWPos.U;
        uint32_t ReadPos = Rb->ReadPos;
        uint32_t BusySize = Curr->S.ReservedPos - ReadPos;
        if (BusySize > BUFFER_SIZE) {
            // Буфер сломан:
            // размер занятой области не может быть больше всего буфера. 
            return -1;
        }
        if (BusySize + Size > BUFFER_SIZE) {
            // Сообщение не поместилось.
            // В зависимости от выбранного поведения, сообщение либо
            // пропускается, либо ожидает завершения записи других.
        }

        ...
    }
}
