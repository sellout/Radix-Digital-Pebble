# A radical watch face for the [Pebble](http://getpebble.com)

## What the heck is this clock displaying?!

The clock is probably useless to anyone but me. I have an odd obsession with numerals and clocks and other such things, and this particular clock represents one point in that obsession-space.

If you sweep away all the arbitrary time constructs we use (weeks, hours, etc.) you are left with a small number of things that actually exist – years (the duration of a planet’s orbit around its sun), days (the duration of a planet’s rotation), and possibly also some lunar cycle. These are all completely independent of each other (some planets have days that are longer than their years) which makes unifying them into one time system rather complex. EG, “month” refers to the lunar cycle, but the month in the modern Gregorian calendar does a pretty poor job of tracking the moon.

This clock tries to generate the simplest system of tracking time. It has two components, the year, and the day. The year is simply an integer, `2013`, as we’re all used to. The day is displayed as `172.9682` – that is the current time in days since the beginning of the year. At this point, you’re probably thinking “well, that’s useless.” I can’t really argue with that, considering the world has agreed that the current system – mixing bases 7, 10, 12, and 60 in various places – is what we should use.

Can this be made reasonable? Let’s try. First, let’s see how to intepret this clock:

    2013 230.8016

The year is easy, it’s 2013, just as we’d expect. Now, 230 (we’ll come back to the fractional part) days since the year started. There’s about 30 days in a month, so 230 / 30 = 7 with a remainder of 20. If Jan 1 is 0, that is roughly August 20. Hey, it’s actually August 19, so not too bad. Now, the fractional part, “8016”. This is a bit messy. 1/10 of a day is 2.4 hours, so 8/10 of a day is 19.2 hours, 0.2 hours is 12 minutes, so it’s 19:12 (7:12 PM) plus whatever .0016 is, which is another 23 minutes, so 19:35. [NB: This particular example has a 0 in the hundredths place, which is unfortunate, because that’s the most useful one – it’s ~15 minutes.]

The four fractional digits represent 2.4 hours, 14.4 minutes, 1.44 minutes, and 8.64 seconds respectively

Well, that was painful. I wonder if we can make it a bit easier. Let’s try base 12.

    11B9 172.9877

Well, now the year is completely opaque. I don’t think the days look any better – nothing like the length of a month or week lines up with base 12. But the fractional part becomes much more readable, “.9877”. 1/12 of a day is 2 hours, so 9/12 is 18:00 (6:00 PM). The next digit magically represents exactly 10 minutes, so 8 of those is 80 minutes, which makes it 19:20, the third digit is 10/12 of a minute, which is close enough to 1 minute for me to round it, and the last is ~4 seconds – and who even cares about the less-than-a-minute that might represent? So, multiply the first digit by 2 hours, the second by 10 minutes, and the third by 1 minute. See, clocks are already pretty well aligned with base 12.

Now me, I’m a base 12 ([dozenal](http://www.dozenal.org/drupal/content/basics)) guy. I think there are lots of good reasons to use it for everything, and not many good reasons to stick with our existing farrago of a time system, so this is where I get off. I use my watch in base 12 mode and love it.

But … on the off chance someone wants to use this and doesn’t really buy into the propaganda, I’ve made it possible to mix and match bases (just like the time system you already know ~~and love~~!). To create something that maps to the existing system, try base 10 for the year and base 12 for “sub-day”. For the integer part of the day, you may like the base 10 version described above, or perhaps base 30 (which would put the (approximate) day of the month in the “ones” column, and the current month in the “30s” column) or base 7 (which would put the current day of the week in the ones column and the week of the year in the other columns).

## Options

There are various options described in `options.h` that you can change to adjust the behavior (EG, the base(s) to use).

## NOTE:

The required font resource isn’t included in the repository. This is because I don’t have a license to distribute the Hoefler Text font. You will have to add this file yourself before building. It is included with Mac OS X. If you don’t have access to this font, you may substitute [something similar](http://graphicdesign.stackexchange.com/questions/17347/can-anyone-suggest-a-free-equivalent-of-the-hoefler-text-font), but it may mean adjusting some of the `TextLayer` rects to make things fit.
