SELECT MODEL202.is_mutagen,MODEL202.lumo, count(distinct MODEL202.model_id ) FROM MODEL MODEL202  WHERE MODEL202.lumo='-2' group by MODEL202.lumo , MODEL202.is_mutagen;
