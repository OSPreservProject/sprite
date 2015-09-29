if (!defined &_LIST) {
    eval 'sub _LIST {1;}';
    if (!defined &_SPRITE) {
    }
    eval 'sub List_InitElement {
        local($elementPtr) = @_;
        eval "($elementPtr)-> &prevPtr = ( &List_Links *)  &NIL; ($elementPtr)-> &nextPtr = ( &List_Links *)  &NIL;";
    }';
    eval 'sub LIST_FORALL {
        local($headerPtr, $itemPtr) = @_;
        eval " &for (($itemPtr) =  &List_First($headerPtr); ! &List_IsAtEnd(($headerPtr),($itemPtr)); ($itemPtr) =  &List_Next($itemPtr))";
    }';
    eval 'sub List_IsEmpty {
        local($headerPtr) = @_;
        eval "(($headerPtr) == ($headerPtr)-> &nextPtr)";
    }';
    eval 'sub List_IsAtEnd {
        local($headerPtr, $itemPtr) = @_;
        eval "(($itemPtr) == ($headerPtr))";
    }';
    eval 'sub List_First {
        local($headerPtr) = @_;
        eval "(($headerPtr)-> &nextPtr)";
    }';
    eval 'sub List_Last {
        local($headerPtr) = @_;
        eval "(($headerPtr)-> &prevPtr)";
    }';
    eval 'sub List_Prev {
        local($itemPtr) = @_;
        eval "(($itemPtr)-> &prevPtr)";
    }';
    eval 'sub List_Next {
        local($itemPtr) = @_;
        eval "(($itemPtr)-> &nextPtr)";
    }';
    eval 'sub LIST_AFTER {
        local($itemPtr) = @_;
        eval "(( &List_Links *) $itemPtr)";
    }';
    eval 'sub LIST_BEFORE {
        local($itemPtr) = @_;
        eval "((( &List_Links *) $itemPtr)-> &prevPtr)";
    }';
    eval 'sub LIST_ATFRONT {
        local($headerPtr) = @_;
        eval "(( &List_Links *) $headerPtr)";
    }';
    eval 'sub LIST_ATREAR {
        local($headerPtr) = @_;
        eval "((( &List_Links *) $headerPtr)-> &prevPtr)";
    }';
}
1;
