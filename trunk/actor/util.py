import logging
import logging.handlers

def defaultLogging(level = logging.DEBUG):
    l = logging.getLogger('')
    l.setLevel(level)
    log_formatter = logging.Formatter(
        '%(asctime)s %(name)s %(levelname)-5s %(message)s')
    hdlr = logging.StreamHandler()
    hdlr.setFormatter(log_formatter)
    l.addHandler(hdlr)

    rhdlr = logging.handlers.RotatingFileHandler('actor.log')
    rhdlr.setFormatter(log_formatter)
    l.addHandler(rhdlr)
    
