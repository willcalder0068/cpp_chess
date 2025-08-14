#pragma once
#include <QObject>
#include <QProcess>
#include <QString>
#include <QDateTime>
#include <QThread>

class StockfishEngine : public QObject {
    Q_OBJECT  // Declare the class as a Qt meta-object, meaning it can be manipulated at runtime (after compilation)
    public:
        // By setting the mainwindow as the parent, it will shut it down when it it closed
        explicit StockfishEngine(QObject* parent=nullptr) : QObject(parent) {}

        bool start(const QString& enginePath) {
            stop();  // Shut down any previous actions

            // Make a QProcess and have it run the stockfish executable
            proc_ = new QProcess(this);
            proc_->setProgram(enginePath);

            proc_->setProcessChannelMode(QProcess::MergedChannels);

            // Launch; if it takes more than four seconds, fold
            proc_->start();
            if (!proc_->waitForStarted(4000)) { return false; }

            // Switch the engine to uci and give it four seconds to confirm
            if (!send("uci") || !waitFor("uciok", 4000)) { return false; }

            // Send basic options to save CPU power
            send("setoption name Threads value 1");
            send("setoption name Hash value 256");
            send("setoption name Ponder value false");
            send("setoption name MultiPV value 1");
            send("setoption name Contempt value 0"); 
            send("setoption name Move Overhead value 0");
            send("setoption name Skill Level value 20");

            // Send the okay and wait for a response
            return isReady(3000);
        }

        void stop() {
            if (!proc_) { return; }  // Nothing to stop, just return

            // Quit and wait for the signal to be recieved
            send("quit");
            proc_->waitForFinished(200);

            // Mark for deletion and clear process + buffer
            proc_->deleteLater();
            proc_ = nullptr;
            buf_.clear();

            // Next time we start it will be a fresh game
            freshGame_ = true;
        }

        bool setElo(int elo) {
            if (elo < 800) elo = 800;
            if (elo > 3200) elo = 3200;

            // Limit strength to requested Elo
            const bool ok =
                send("setoption name UCI_LimitStrength value true")       &&
                send(QString("setoption name UCI_Elo value %1").arg(elo)) &&
                isReady(2000);

            // If properly executed, return true
            return ok;
        }

        QString bestMove(const QString& fen, int movetimeMs = 1000) {
            if (freshGame_) {
                // Send ucinewgame to stockfish so it can clear transposition table
                if (!send("ucinewgame") || !isReady(2000)) { return {}; }

                // Flip
                freshGame_ = false;
            }
            buf_.clear();

            // Sends the fen position and the movetime to stockfish
            if (!send(QString("position fen %1").arg(fen))) { return {}; }
            if (!send(QString("go movetime %1").arg(movetimeMs))) { return {}; }

            // Returns the best move
            return waitBestMove(movetimeMs + 5000);
        }

        // Call this when you start a brand new game (no arg changes elsewhere).
        void markNewGame() { 
            freshGame_ = true; 
        }

    private:
        // Declare class variables
        QProcess* proc_ = nullptr;
        QString buf_;
        bool freshGame_ = true;

        // Time helper
        static qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }

        bool send(const QString& s) {
            if (!proc_) { return false; }
            // Write a newline (uci requires utf8)
            const QByteArray b = (s + "\n").toUtf8();
            if (proc_->write(b) == -1) { return false; }

            // Waits one second to be accepted
            return proc_->waitForBytesWritten(1000);
        }

        bool isReady(int timeoutMs) {
            if (!send("isready")) { return false; }

            return waitFor("readyok", timeoutMs);
        }

        bool waitFor(const QString& token, int timeoutMs) {
            const qint64 end = nowMs() + timeoutMs;  // Calculate endtime
            while (nowMs() < end) {
                if (!proc_->waitForReadyRead(50)) { continue; }  // Wait for new data
                buf_ += QString::fromUtf8(proc_->readAll());  // Append new data to the buffer

                int nl;
                while ((nl = buf_.indexOf('\n')) != -1) {  // Read line by line
                    const QString line = buf_.left(nl).trimmed();

                    buf_.remove(0, nl + 1);  // Remove each line
                    if (line == token) { return true; }  // If it is token, success
                }
            }

            return false;
        }

        QString waitBestMove(int timeoutMs) {
            const qint64 end = nowMs() + timeoutMs;  // Calculate endtime
            while (nowMs() < end) {
                if (!proc_->waitForReadyRead(50)) { continue; }  // Wait for new data
                buf_ += QString::fromUtf8(proc_->readAll());  // Append new data to the buffer

                int nl;
                while ((nl = buf_.indexOf('\n')) != -1) {  // Read line by line
                    const QString line = buf_.left(nl).trimmed();

                    buf_.remove(0, nl + 1);  // Remove each line
                    if (line.startsWith("bestmove ")) {
                        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                        return (parts.size() >= 2) ? parts[1] : QString{};  // Return the move in uci
                    }
                }
            }
            return {};
        }
};