#include "process_queries.h"
#include "search_server.h"

#include <execution>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main() {
	SearchServer search_server("and with"s);

	int id = 0;
	for (
		const string& text : {
			"If you can keep your head when all about you"s,
			"Are losing theirsand blaming it on you,"s,
			"If you can trust yourself when all men doubt you,"s,
			"But make allowance for their doubting too;"s,
			"If you can wait and not be tired by waiting,"s,
			"Or being lied about, don’t deal in lies,"s,
			"Or being hated don’t give way to hating,"s,
			"And yet don’t look too good, nor talk too wise :"s,
			"If you can dream - and not make dreams your master;"s,
			"If you can think - and not make thoughts your aim,"s,
			"If you can meet with Triumphand Disaster"s,
			"And treat those two impostors just the same;"s,
			"If you can bear to hear the truth you’ve spoken"s,
			"Twisted by knaves to make a trap for fools,"s,
			"Or watch the things you gave your life to, broken,"s,
			"And stoopand build ’em up with worn - out tools :"s,
			"If you can make one heap of all your winnings"s,
			"And risk it on one turn of pitch - and -toss,"s,
			"And lose,and start again at your beginnings"s,
			"And never breathe a word about your loss;"s,
			"If you can force your heartand nerveand sinew"s,
			"To serve your turn long after they are gone,"s,
			"And so hold on when there is nothing in you"s,
			"Except the Will which says to them : ‘Hold on!’"s,
			"If you can talk with crowdsand keep your virtue,"s,
			"Or walk with Kings - nor lose the common touch,"s,
			"If neither foes nor loving friends can hurt you,"s,
			"If all men count with you, but none too much;"s,
			"If you can fill the unforgiving minute"s,
			"With sixty seconds’ worth of distance run,"s,
			"Yours is the Earthand everything that’s in it,"s,
			"And - which is more - you’ll be a Man, my son!"s,
		}
		) {
		search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	}


	cout << "ACTUAL by default, execution::seq:"s << endl;
	// последовательная версия
	for (const Document& document : search_server.FindTopDocuments("If you can"s)) {
		PrintDocument(document);
	}
	cout << "\nBANNED, execution::seq:"s << endl;
	// последовательная версия
	for (const Document& document : search_server.FindTopDocuments(execution::seq, "If you can"s, DocumentStatus::BANNED)) {
		PrintDocument(document);
	}

	cout << "\nACTUAL by, even ids, execution::par:"s << endl;
	// параллельная версия
	for (const Document& document
		: search_server.FindTopDocuments(execution::par,
			"If you can"s,
			[](int document_id, DocumentStatus status, int rating) {
				return document_id % 2 == 0;
			}))
	{
		PrintDocument(document);
	}

			return 0;
}