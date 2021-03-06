#include "inputs/ssh.h"

#include <QStringList>
#include <iostream>
#include "glwidget.h"

using namespace std;

void SSH::startProcessing(void) {
  proc = new QProcess( this );

  proc->setProcessChannelMode(QProcess::SeparateChannels);

  connect( proc, SIGNAL(readyReadStandardOutput()), this, SLOT(readFromStdout()) );
  connect( proc, SIGNAL(readyReadStandardError()), this, SLOT(readFromStderr()) );
  QString cmd = getCommandString();
  std::cout << "[" << domain.toStdString() << "] exec[" << cmd.toStdString() << "]" << std::endl;
  proc->start( getCommandString() );
  proc->waitForStarted();
}


void SSH::endProcessing( void ) {
  cout << "Terminating[" << domain.toStdString() << "]" << endl;
  proc->terminate();
  proc->waitForFinished(2000);
  if( proc->state() == QProcess::Running ) {
    cout << "Killing[" << domain.toStdString() << "]" << endl;
    proc->kill();
  }

}

QString SSH::getCommandString() {
  QStringList cmd;
  cmd << "ssh";
  cmd << user + "@" + host;

  if( !port.isEmpty() )
    cmd << "-p" << port;

  cmd << command << args;

  return cmd.join(" ");
}

void SSH::readFromStdout(void ) {
  char buf[2048];

  proc->setReadChannel(QProcess::StandardOutput);

  while( proc->canReadLine() ) {
    qint64 len = proc->readLine(buf, sizeof(buf));
    if( len != -1 ) {

      //      QString apache_pattern("([\\d\\S.]+) (\\S+) (\\S+) \\[([^\\]]+)\\] \"([^\"]*)\" (\\d+) ([\\S]+) \"([^\"]+)\" \"([^\"]+)\"");
      QRegExp rx(pattern);

      if( rx.indexIn(buf) > -1 ) {
        QString url = rx.cap(1);
        QString referrer = rx.cap(2);

        if( url.contains(" ") ) {
          url = url.split(" ")[1];
        }

        if( ignoreQueryParameters() )
          url = url.split("?")[0];

        if( !ignore.isEmpty() && url.contains( QRegExp(ignore, Qt::CaseInsensitive) ) ) {
          continue;
        }

        for( int i = 0; i < replacementPatterns.size(); i++ ) {
          url.replace( QRegExp(replacementPatterns[i]), replacementStrings[i] );
        }

        url.replace( QRegExp("/index\\.vsp$"), "/" );

        if( referrer.contains( QRegExp("http://(.*\\.)?" + domain) ) || referrer.startsWith("/") )  {
            referrer = referrer.split("?")[0];

            for( int i = 0; i < replacementPatterns.size(); i++ ) {
              referrer.replace( QRegExp(replacementPatterns[i]), replacementStrings[i] );
            }
        }

        referrer.replace( QRegExp("^http://(.*\\.)?" + domain), "");
        bool external = false;
        if( referrer.contains("http://") ) {
          external = true;
          referrer.replace("http://", "");
        }
        //        std::cout << "[" << domain.toStdString() << "] " <<  url.toStdString() << " <- " << (const char*) referrer.toLatin1() << std::endl;

        if( url == referrer ) {
          referrer = "";
        }

        gl->addRelation(this, url, referrer, external);

      } else {
        std::cout << "[" << domain.toStdString() << "] UNKNOWN [" << buf << "]" << std::endl;
      }
      //      glWidget->addRandomElement();
    }
  }
}

void SSH::readFromStderr(void) {
  char buf[2048];
  std::cout << "STDERR!\n";
  proc->setReadChannel(QProcess::StandardError);

  while( proc->canReadLine() ) {
    qint64 len = proc->readLine(buf, sizeof(buf));
    if( len != -1 ) {
      std::cout << "[" << domain.toStdString() << "] STDERR [" << buf << "]" << std::endl;
    }
  }
}
