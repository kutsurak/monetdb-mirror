SELECT MODEL247.is_mutagen, count(distinct MODEL247.model_id ) FROM MODEL MODEL247, ATOM T1008290427740  WHERE MODEL247.model_id=T1008290427740.model_id AND MODEL247.logp='6' group by MODEL247.is_mutagen;
